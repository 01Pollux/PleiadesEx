
#include "CallContext.hpp"

namespace ShadowGarden
{
	CallContext::CallContext(InitToken&& token) :
		m_FuncSig(token.FuncSig),
		m_PassRet(std::move(token.RetData)),
		m_PassArgs(std::move(token.ArgsData))
	{ }

	/// <summary>
	/// https://github.com/asmjit/asmjit/blob/master/src/asmjit/x86/x86emithelper.cpp#L171
	/// </summary>
	static JIT::Error EmitRegMove(JIT::x86::Compiler& comp, const JIT::Operand& dst_, const JIT::Operand& src_, uint32_t typeId);


	void CallContext::ManageArgs(const DetourDetail::TypeInfo& typeInfo, JIT::x86::Compiler& comp, bool load_from_compiler)
	{
		using namespace JIT;

		size_t arg_pos = 0;
		if (typeInfo.has_this_ptr())
		{
			auto& cur = m_PassArgs.m_CurData[arg_pos++];
			x86::Mem mem(uint32_t(cur.data()), cur.size());
			typeInfo.this_().id();

			if (load_from_compiler)
			{
				EmitRegMove(comp, mem, typeInfo.this_(), comp.virtRegByReg(typeInfo.this_())->typeId());
			}
			else
			{
				EmitRegMove(comp, typeInfo.this_(), mem, comp.virtRegByReg(typeInfo.this_())->typeId());
			}
		}

		for (size_t offset = 0; auto& arg : typeInfo.args_iterator())
		{
			auto& cur = m_PassArgs.m_CurData[arg_pos];
			x86::Mem mem(uint64_t(cur.data()) + offset);

			if (load_from_compiler)
			{
				EmitRegMove(comp, mem, arg.Reg, comp.virtRegByReg(arg.Reg)->typeId());
			}
			else
			{
				EmitRegMove(comp, arg.Reg, mem, comp.virtRegByReg(arg.Reg)->typeId());
			}

			if (arg.ExtraReg.isValid())
			{
				offset += arg.Size;
				const x86::Mem mem_p4(mem.cloneAdjusted(4));

				if (load_from_compiler)
				{
					EmitRegMove(comp, arg.ExtraReg, mem_p4, comp.virtRegByReg(arg.ExtraReg)->typeId());
				}
				else
				{
					EmitRegMove(comp, mem_p4, arg.ExtraReg, comp.virtRegByReg(arg.ExtraReg)->typeId());
				}
			}

			if (offset += arg.Size; offset >= cur.size())
			{
				offset = 0;
				++arg_pos;
				continue;
			}
		}
	}

	void CallContext::WriteChangedArgs()
	{
		const size_t size = m_PassArgs.size(), offset{ };
		for (size_t i = 0; i < size; i++)
		{
			if (m_PassArgs.has_changed(i))
			{
				const auto& to = m_PassArgs.m_CurData[i], &from = m_PassArgs.m_NewData[i];
				memcpy(to.data(), from.data(), from.size());
			};
		}
	}

	void CallContext::WriteChangedReturn()
	{
		if (!m_PassRet.has_changed() && !m_PassRet.is_void())
			return;

		auto& to = m_PassRet.m_CurData,& from = m_PassRet.m_NewData;
		memcpy(to.data(), from.data(), from.size());
	}

	void CallContext::ManageReturn(JIT::x86::Compiler& comp, bool read_from_compiler, const JIT::BaseReg& reg0, const JIT::BaseReg& reg1)
	{
		using namespace JIT;
		using RegGroup = x86::Reg::RegGroup;

		auto& out_addr = m_PassRet.m_CurData;
		x86::Mem mem(uint64_t(out_addr.data()), out_addr.size());

		if (read_from_compiler)
		{
			EmitRegMove(comp, mem, reg0, comp.virtRegByReg(reg0)->typeId());
			if (reg1.isValid())
			{
				const x86::Mem mem_p4(mem.cloneAdjusted(4));
				EmitRegMove(comp, mem_p4, reg1, comp.virtRegByReg(reg1)->typeId());
			}
		}
		else
		{
			EmitRegMove(comp, reg0, mem, comp.virtRegByReg(reg0)->typeId());
			if (reg1.isValid())
			{
				const x86::Mem mem_p4(mem.cloneAdjusted(4));
				EmitRegMove(comp, reg1, mem_p4, comp.virtRegByReg(reg1)->typeId());
			}
		}
	}

	void CallContext::ManageReturnInMem(JIT::x86::Compiler& comp, bool read_from_compiler, const JIT::BaseReg& outreg)
	{
		using namespace JIT;

		InvokeNode* pFunc;
		comp.invoke(&pFunc, memcpy, FuncSignatureT<void*, void*, void const*, size_t>());
		
		if (read_from_compiler)
		{
			pFunc->setArg(0, m_PassRet.m_CurData.data());
			pFunc->setArg(1, outreg);
		}
		else
		{
			pFunc->setArg(0, outreg);
			pFunc->setArg(1, m_PassRet.m_CurData.data());
		}
		
		pFunc->setArg(2, m_PassRet.size());
	}


	void CallContext::SaveReturn()
	{
		auto& ret_p = m_PassRet.m_CurData;
		if (!ret_p.is_void())
		{
			const size_t size = ret_p.size();
			auto ret(std::make_unique<PassRet::data_arr>(size));
			memcpy(ret.get(), ret_p.data(), size);
			m_SavedRets.push(std::move(ret));
		}
	}

	void CallContext::RestoreReturn()
	{
		auto& ret_p = m_PassRet.m_CurData;
		if (!ret_p.is_void())
		{
			const size_t size = ret_p.size();
			auto& ret = m_SavedRets.top();
			memcpy(ret_p.data(), ret.get(), size);
			m_SavedRets.pop();
		}
	}

	void CallContext::ResetState()
	{
		m_PassRet.m_Changed = false;
		for (size_t i = 0; i < m_PassArgs.size(); i++)
			m_PassArgs.m_ArgInfo[i].Changed = false;
	}


	static JIT::Error EmitRegMove(JIT::x86::Compiler& comp, const JIT::Operand& dst_, const JIT::Operand& src_, uint32_t typeId)
	{
		using namespace JIT;
		using namespace JIT::x86;

		Operand dst(dst_);
		Operand src(src_);

		uint32_t instId = Inst::kIdNone;
		uint32_t memFlags = 0;
		uint32_t overrideMemSize = 0;

		enum MemFlags : uint32_t
		{
			kDstMem = 0x1,
			kSrcMem = 0x2
		};

		const bool is_avx_on = comp.func()->frame().isAvxEnabled();

		// Detect memory operands and patch them to have the same size as the register.
		// BaseCompiler always sets memory size of allocs and spills, so it shouldn't
		// be really necessary, however, after this function was separated from Compiler
		// it's better to make sure that the size is always specified, as we can use
		// 'movzx' and 'movsx' that rely on it.
		if (dst.isMem()) { memFlags |= kDstMem; dst.as<Mem>().setSize(src.size()); }
		if (src.isMem()) { memFlags |= kSrcMem; src.as<Mem>().setSize(dst.size()); }

		switch (typeId)
		{
		case Type::kIdI8:
		case Type::kIdU8:
		case Type::kIdI16:
		case Type::kIdU16:
			// Special case - 'movzx' load.
			if (memFlags & kSrcMem)
			{
				instId = Inst::kIdMovzx;
				dst.setSignature(Reg::signatureOfT<Reg::kTypeGpd>());
			}
			else if (!memFlags)
			{
				// Change both destination and source registers to GPD (safer, no dependencies).
				dst.setSignature(Reg::signatureOfT<Reg::kTypeGpd>());
				src.setSignature(Reg::signatureOfT<Reg::kTypeGpd>());
			}
			__fallthrough;

		case Type::kIdI32:
		case Type::kIdU32:
		case Type::kIdI64:
		case Type::kIdU64:
			instId = Inst::kIdMov;
			break;

		case Type::kIdMmx32:
			instId = Inst::kIdMovd;
			if (memFlags) break;
			__fallthrough;

		case Type::kIdMmx64: instId = Inst::kIdMovq; break;
		case Type::kIdMask8: instId = Inst::kIdKmovb; break;
		case Type::kIdMask16: instId = Inst::kIdKmovw; break;
		case Type::kIdMask32: instId = Inst::kIdKmovd; break;
		case Type::kIdMask64: instId = Inst::kIdKmovq; break;

		default:
		{
			const uint32_t elementTypeId = Type::baseOf(typeId);
			if (Type::isVec32(typeId) && memFlags)
			{
				overrideMemSize = 4;
				if (elementTypeId == Type::kIdF32)
					instId = is_avx_on ? Inst::kIdVmovss : Inst::kIdMovss;
				else
					instId = is_avx_on ? Inst::kIdVmovd : Inst::kIdMovd;
				break;
			}

			if (Type::isVec64(typeId) && memFlags)
			{
				overrideMemSize = 8;
				if (elementTypeId == Type::kIdF64)
					instId = is_avx_on ? Inst::kIdVmovsd : Inst::kIdMovsd;
				else
					instId = is_avx_on ? Inst::kIdVmovq : Inst::kIdMovq;
				break;
			}

			if (elementTypeId == Type::kIdF32)
				instId = is_avx_on ? Inst::kIdVmovaps : Inst::kIdMovaps;
			else if (elementTypeId == Type::kIdF64)
				instId = is_avx_on ? Inst::kIdVmovapd : Inst::kIdMovapd;
			else if (!comp.func()->frame().isAvx512Enabled())
				instId = is_avx_on ? Inst::kIdVmovdqa : Inst::kIdMovdqa;
			else
				instId = Inst::kIdVmovdqa32;
			break;
		}
		}

		if (!instId)
			return DebugUtils::errored(kErrorInvalidState);

		if (overrideMemSize)
		{
			if (dst.isMem()) dst.as<Mem>().setSize(overrideMemSize);
			if (src.isMem()) src.as<Mem>().setSize(overrideMemSize);
		}

		return comp.emit(instId, dst, src);
	}
}