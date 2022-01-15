#include <nlohmann/json.hpp>

#include "HookInstance.hpp"
#include "SigBuilder.hpp"

#include "library/Manager.hpp"
#include "logs/Logger.hpp"

HookInstance::HookInstance(px::IntPtr original_function, const nlohmann::json& data, std::string& out_err) :
	m_AddressInMemory(original_function)
{
	detour_detail::SigBuilder sig(data, out_err);
	if (out_err.empty())
	{
		void* callback = AllocCallbackHandler(sig, out_err);
		if (!callback)
			return;

		if (const LONG res = m_Detour.attach(original_function.get(), callback); res)
		{
			if (callback)
				px::lib_manager.GetRuntime()->release(callback);

			std::format_to(std::back_inserter(out_err), "Failed to detour the function (Code: {})", res);
		}
	}
}

void HookInstance::ClearCallbacks() noexcept
{
	std::lock_guard lock(m_CallbacksLock);
	m_PreCallbacks.clear();
	m_PostCallbacks.clear();
}

void HookInstance::Activate()
{
	void* callback = m_Detour.callback_function();
	assert(!m_Detour.is_set() && callback);
	if (callback)
	{
		m_Detour.attach();
	}
}

void HookInstance::Deactivate()
{
	void* callback = m_Detour.callback_function();
	assert(m_Detour.is_set() && callback);
	if (callback)
	{
		this->ClearCallbacks();
		m_Detour.detach();
	}
	
}

void* HookInstance::GetRuntimeCallback()
{
	return m_Detour.callback_function();
}

void* HookInstance::AllocCallbackHandler(detour_detail::SigBuilder& sigbuilder, std::string& out_err)
{
	using namespace asmjit;

	CodeHolder code;
	code.init(px::lib_manager.GetRuntime()->environment());

	x86::Compiler comp(&code);
	comp.mov(x86::Mem(uint64_t(this) + offsetof(HookInstance, m_StackPointer)), x86::esp);

	detour_detail::TypeInfo typeInfo;
	m_CallContext = sigbuilder.load_args(comp, typeInfo);

	x86::Gp should_call_func = comp.newInt8();
	const Label L_PostCode = comp.newLabel();

	DataInfo info{
		.typeInfo = typeInfo,
		.Compiler = comp
	};

	if (!ValidateRegisters(info, out_err))
		return nullptr;

	this->InvokeCallbacks(info, false, should_call_func);
	comp.cmp(should_call_func, 1);

	comp.jnz(L_PostCode);

	this->InvokeOriginal(info);

	this->ReadReturn(info);

	comp.bind(L_PostCode);

	this->InvokeCallbacks(info, true, should_call_func);

	this->WriteReturn(info);

	comp.endFunc();
	if (const auto err = comp.finalize())
	{
		std::format_to(std::back_inserter(out_err), "Failed to finalize the x86::Compiler (Code: {})", err);
		return nullptr;
	}

	void* fn;
	if (const auto err = px::lib_manager.GetRuntime()->add(&fn, &code))
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
	using px::MHookRes;
	using px::HookRes;

	const auto handle_callbacks = [is_post, this] ()
	{
		std::lock_guard lock(m_CallbacksLock);

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

		if (last.test(HookRes::ChangedReturn))
			m_CallContext->PopReturn();
		m_CallContext->PopArgs();

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
		bool do_call = true;

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
				m_CallContext->PushReturn();
			}
			do_call = !res.test(HookRes::DontCall);
		}

		m_CallContext->PushArgs();

		return do_call;
	}
}


void HookInstance::InvokeCallbacks(DataInfo info, bool post, const asmjit::x86::Gp& ret)
{
	using namespace asmjit;
	const auto handler_fn = &HookInstance::RunHandler;

	// we don't want to reload args two times, we will just do once it in pre hooks
	if (!post)
		m_CallContext->ManageArgs(info.typeInfo, info.Compiler, true);

	InvokeNode* pFunc;
	info.Compiler.invoke(&pFunc, std::bit_cast<void*>(handler_fn), FuncSignatureT<bool, HookInstance*, bool>(CallConvId::kThisCall));

	pFunc->setArg(0, this);
	pFunc->setArg(1, post);
	pFunc->setRet(0, ret);

	if (!post)
		m_CallContext->ManageArgs(info.typeInfo, info.Compiler, false);
}


void HookInstance::InvokeOriginal(DataInfo info)
{
	using namespace asmjit;

	InvokeNode* pFunc;
	info.Compiler.invoke(&pFunc, x86::Mem(uint64_t(this) + offsetof(HookInstance, m_Detour) + detour_detail::Detour::offset_to_m_ActualFunc()), m_CallContext->m_FuncSig);

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
	using namespace asmjit;

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
	using namespace asmjit;

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


px::IHookInstance::HookID HookInstance::AddCallback(bool post, px::HookOrder order, const CallbackType& callback)
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

void HookInstance::RemoveCallback(bool post, HookID id)
{
	auto& sets = post ? m_PostCallbacks : m_PreCallbacks;
	std::erase_if(sets, [id] (const HookInfo& o) { return o == id; });
}


px::MHookRes HookInstance::GetLastResults() noexcept
{
	return this->m_LastResults.size() ? this->m_LastResults.top() : px::MHookRes{ };
}

void* HookInstance::GetFunction() noexcept
{
	return m_Detour.original_function();
}

void* HookInstance::GetStackPointer() noexcept
{
	return m_StackPointer;
}
