#include <nlohmann/Json.hpp>

#include "HookInstance.hpp"
#include "SigBuilder.hpp"

#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Interfaces/Logger.hpp"


SG_NAMESPACE_BEGIN;

HookInstance::HookInstance(IntPtr original_function, const Json& data, std::string& out_err) :
	m_AddressInMemory(original_function)
{
	DetourDetail::SigBuilder sig(data, out_err);
	if (out_err.empty())
	{
		void* callback = AllocCallbackHandler(sig, out_err);
		if (!callback)
			return;

		if (const LONG res = m_Detour.attach(original_function.get(), callback); res)
		{
			if (callback)
				SG::lib_manager.GetRuntime()->release(callback);

			std::format_to(std::back_inserter(out_err), "Failed to detour the function (Code: {})", res);
		}
		else
		{
			m_ActualFunction = m_Detour.original_function();
		}
	}
}

HookInstance::~HookInstance() noexcept
{
	void* callback = m_Detour.callback_function();
	if (callback)
	{
		m_Detour.detach();
		SG::lib_manager.GetRuntime()->release(callback);
	}
}

void* HookInstance::AllocCallbackHandler(DetourDetail::SigBuilder& sigbuilder, std::string& out_err)
{
	using namespace JIT;

	CodeHolder code;
	code.init(SG::lib_manager.GetRuntime()->environment());

	x86::Compiler comp(&code);

	DetourDetail::TypeInfo typeInfo;
	m_CallContext = sigbuilder.load_args(comp, typeInfo);

	const Label L_PostCode = comp.newLabel();

	x86::Gp hook_ret = comp.newInt8();

	DataInfo info{
		.typeInfo = typeInfo,
		.Compiler = comp
	};

	if (!ValidateRegisters(info, out_err))
		return nullptr;

	this->InvokeCallbacks(info, false, hook_ret);
	comp.test(hook_ret, hook_ret);

	comp.jnz(L_PostCode);

	this->InvokeOriginal(info);

	this->ReadReturn(info);

	comp.bind(L_PostCode);

	this->InvokeCallbacks(info, true, hook_ret);

	this->WriteReturn(info);

	comp.endFunc();
	if (const auto err = comp.finalize())
	{
		std::format_to(std::back_inserter(out_err), "Failed to finalize the x86::Compiler (Code: {})", err);
		return nullptr;
	}

	void* fn;
	if (const auto err = SG::lib_manager.GetRuntime()->add(&fn, &code))
	{
		std::format_to(std::back_inserter(out_err), "Failed to add the function to JIT runtime (Code: {})", err);
		return nullptr;
	}

	return fn;
}

bool HookInstance::ValidateRegisters(DataInfo info, std::string& out_err)
{
	const auto& comp = info.Compiler;
	const auto& typeInfo = info.typeInfo;

	if (typeInfo.has_ret())
	{
		if (!comp.isVirtRegValid(typeInfo.ret()))
		{
			out_err = "Invalid return value register id";
			return false;
		}

		if (typeInfo.has_regx2())
		{
			if (!comp.isVirtRegValid(typeInfo.ret(true)))
			{
				out_err = "Invalid return value register id (2nd register)";
				return false;
			}
		}
	}

	if (typeInfo.has_this_ptr())
	{
		if (!comp.isVirtRegValid(typeInfo.this_()))
		{
			out_err = "Invalid |this| pointer register id";
			return false;
		}
	}

	for (auto& arg : typeInfo.args_iterator())
	{
		if (!comp.isVirtRegValid(arg.Reg) || (arg.ExtraReg.isValid() && !comp.isVirtRegValid(arg.ExtraReg)))
		{
			out_err = "Invalid arg register id";
			return false;
		}
	}
	return true;
}

bool HookInstance::RunHandler(bool is_post)
{
	m_CallContext->ResetState();

	const auto handle_callbacks = [is_post, this] ()
	{
		MHookRes highest;
		auto& sets = is_post ? m_PostCallbacks : m_PreCallbacks;
		for (auto& hook : sets)
		{
			MHookRes cur = hook.Callback(&m_CallContext->m_PassRet, &m_CallContext->m_PassArgs);
			if (!highest.test(cur))
			{
				highest |= cur;
				if (cur.test(HookRes::BreakLoop))
					break;
			}
		}
		return highest;
	};

	// 2
	if (is_post)
	{
		const MHookRes last = m_LastResults.top();

		// restore any old return back from the stack
		if (last.test(HookRes::ChangedReturn))
			m_CallContext->RestoreReturn();

		// call post hooks
		if (!last.test(HookRes::SkipPost))
		{
			const MHookRes res = handle_callbacks();

			// did we change any return value?
			if (!res.test(HookRes::Ignored) && res.test(HookRes::ChangedReturn))
			{
				// was 'HookRes::IgnorePostReturn' flag set? if not then change return value
				if (!last.test(HookRes::IgnorePostReturn) || !last.test(HookRes::ChangedReturn))
					m_CallContext->WriteChangedReturn();
			}
		}

		m_LastResults.pop();
		return true;
	}
	// 1 
	else
	{
		const MHookRes res = handle_callbacks();
		m_LastResults.push(res);

		// check if we should ignore anything
		if (!res.test(HookRes::Ignored))
		{
			// check if we changed any params
			if (res.test(HookRes::ChangedParams))
				m_CallContext->WriteChangedArgs();

			// check if we changed any return value
			if (res.test(HookRes::ChangedReturn))
			{
				m_CallContext->WriteChangedReturn();
				m_CallContext->SaveReturn();
			}
			return res.test(HookRes::DontCall);
		}
		else return false;
	}
	return true;
}


void HookInstance::InvokeCallbacks(DataInfo info, bool post, const JIT::x86::Gp& ret)
{
	using namespace JIT;
	const auto handler_fn = &HookInstance::RunHandler;

	// we don't want to reload args two times, we will just do once it in pre hooks
	if (!post)
		m_CallContext->ManageArgs(info.typeInfo, info.Compiler, true);

	InvokeNode* pFunc;
	info.Compiler.invoke(&pFunc, std::bit_cast<void*>(handler_fn), FuncSignatureT<bool, HookInstance*, bool>(CallConv::kIdThisCall));

	pFunc->setArg(0, this);
	pFunc->setArg(1, post);
	pFunc->setRet(0, ret);

	if (!post)
		m_CallContext->ManageArgs(info.typeInfo, info.Compiler, false);
}


void HookInstance::InvokeOriginal(DataInfo info)
{
	using namespace JIT;

	InvokeNode* pFunc;
	info.Compiler.invoke(&pFunc, x86::Mem(uint64_t(this) + offsetof(HookInstance, m_ActualFunction)), m_CallContext->m_FuncSig);

	{
		size_t arg_pos = 0;
		// first check if it's thiscall, then set it as first arg
		if (info.typeInfo.has_this_ptr())
		{
			pFunc->setArg(arg_pos++, info.typeInfo.this_());
		}
		// second check if it's return on the stack, then set it as (first/second) arg
		if (info.typeInfo.has_ret_mem())
		{
			pFunc->setArg(arg_pos++, info.typeInfo.ret_mem());
		}
		// third set rest of args, and keep checking if it's a int64_t type to set second param on 'setArg'
		for (auto& arg : info.typeInfo.args_iterator())
		{
			pFunc->setArg(arg_pos, arg.Reg);
			if (arg.ExtraReg.isValid())
				pFunc->setArg(arg_pos, 1, arg.ExtraReg);
			++arg_pos;
		}
	}

	// if it contains a return, grab it
	if (info.typeInfo.has_ret())
	{
		const BaseReg& ret0 = info.typeInfo.ret();
		pFunc->setRet(0, ret0);

		const BaseReg& ret1 = info.typeInfo.has_regx2() ? info.typeInfo.ret(true) : BaseReg{ };
		if (ret1.isValid())
			pFunc->setRet(1, ret1);
	}
}


void HookInstance::ReadReturn(DataInfo info)
{
	using namespace JIT;

	if (info.typeInfo.has_ret_mem())
	{
		m_CallContext->ManageReturnInMem(info.Compiler, true, info.typeInfo.ret_mem());
	}
	else if (info.typeInfo.has_ret())
	{
		const BaseReg& 
			ret0 = info.typeInfo.ret(),
			ret1 = info.typeInfo.has_regx2() ? info.typeInfo.ret(true) : BaseReg{ };

		m_CallContext->ManageReturn(info.Compiler, true, ret0, ret1);
	}
}

void HookInstance::WriteReturn(DataInfo info)
{
	using namespace JIT;

	if (info.typeInfo.has_ret_mem())
	{
		const BaseReg ret = info.typeInfo.ret_mem();
		m_CallContext->ManageReturnInMem(info.Compiler, false, ret);
	}
	else if (info.typeInfo.has_ret())
	{
		BaseReg
			ret0 = info.typeInfo.ret(),
			ret1 = info.typeInfo.ret(true);

		m_CallContext->ManageReturn(info.Compiler, false, ret0, ret1);
		info.Compiler.addRet(ret0, info.typeInfo.has_regx2() ? ret1 : Operand{ });
	}
}


IHookInstance::HookID HookInstance::AddCallback(bool post, HookOrder order, const CallbackType& callback)
{
	auto& sets = post ? m_PostCallbacks : m_PreCallbacks;

	IHookInstance::HookID id = 0;
	for (const auto& entry : sets)
	{
		if (id == entry.Id)
			id = entry.Id + 1;
	}
	sets.emplace(callback, order, id);

	return id;
}

void HookInstance::RemoveCallback(bool post, HookID id) noexcept
{
	auto& sets = post ? m_PostCallbacks : m_PreCallbacks;
	std::erase_if(sets, [id] (const HookInfo& o) { return o == id; });
}


MHookRes HookInstance::GetLastResults() noexcept
{
	return this->m_LastResults.size() ? this->m_LastResults.top() : MHookRes{ };
}

void* HookInstance::GetFunction() noexcept
{
	return m_ActualFunction;
}

SG_NAMESPACE_END;