#include <fstream>

#include "SigBuilder.hpp"
#include "CallContext.hpp"

#include "Impl/Interfaces/Logger.hpp"


namespace ShadowGarden::DetourDetail
{
	SigBuilder::SigBuilder(const Json& sig_data, std::string& err) :
		m_SigInfo(sig_data)
	{
		using namespace JIT;
		// return value = the value to be inserted in funcsig
		// &type = the value to be inserted with 'x86::Compiler::_newReg()' in the future
		//
		// the function will be used later on for resolving 'm_RetTypes' and 'm_ArgTypes'
		const auto resolve_type =
			[] (uint32_t& type) -> uint32_t
		{
			// ints, only deabstract [U]InPtr
			if (type <= Type::_kIdIntEnd)
			{
				type = Type::deabstract(type, Type::deabstractDeltaOfSize(sizeof(void*)));
				return type;
			}

			switch (type)
			{
			case Type::kIdF32:
			case Type::kIdF32x1:
			{
				type = Type::kIdF32x1;
				return Type::kIdF32;
			}
			case Type::kIdF64:
			case Type::kIdF64x1:
			{
				type = Type::kIdF64x1;
				return Type::kIdF64;
			}
			case Type::kIdF80:
			{
				return Type::kIdF80;
			}

			default:
			{
				if (type >= Type::_kIdMmxStart && type <= Type::_kIdMmxEnd)
				{
					return Type::IdOfT<x86::Mm>::kTypeId;
				}
				else if (type >= Type::_kIdVec32Start && type <= Type::_kIdVec128End)
				{
					return Type::IdOfT<x86::Xmm>::kTypeId;
				}
				else if (type >= Type::_kIdVec256Start && type <= Type::_kIdVec256Start)
				{
					return Type::IdOfT<x86::Ymm>::kTypeId;
				}
				else if (type >= Type::_kIdVec512Start && type <= Type::_kIdVec512Start)
				{
					return Type::IdOfT<x86::Zmm>::kTypeId;
				}
				else
					// !!!
					return 0;
			}
			}
		};

		if (sig_data.empty())
		{
			err = "Detour Entry is empty.";
			return;
		}

		SetCallConv(sig_data["callConv"].get_ref<const std::string&>());
		if (!m_FuncSig.callConv())
		{
			err = "Invalid Callconv type.";
			return;
		}

		// Read return types
		{
			// support for faster type access instead of it being registered in 'Pleiades.JITTypes.json'
			m_RetTypes = m_Types.load_type(sig_data["return"]);
			
			if (m_RetTypes.empty())
			{
				err = "Invalid return type";
				return;
			}

			// if (m_RetTypes.size() > 1) we wouldn't care about 'final_ret_sig'
			uint32_t final_ret_sig{ };
			for (uint32_t& type : m_RetTypes)
				final_ret_sig = resolve_type(type);

			// if the type's size is bigger than 1 type, it should return in the stack, else we will use m_RetType[0] (either void, int, etc)
			if (m_RetTypes.size() > 1)
			{
				m_FuncSig.setRetT<void>();
				m_FuncSig.addArgT<void*>();
			}
			else
			{
				m_FuncSig.setRet(final_ret_sig);
			}
		}

		// Read the args types
		{
			// if the callconv is thiscall, use void* for |this| pointer first param
			if (m_FuncSig.callConv() == CallConv::kIdThisCall)
				m_FuncSig.addArg(Type::kIdUIntPtr);

			if (const auto args = sig_data.find("Arguments"); args != sig_data.end())
			{
				m_ArgTypes.reserve(args->size());
				for (auto& arg : *args)
				{
					auto& fullarg = m_ArgTypes.emplace_back(m_Types.load_type(arg["type"]), arg.contains("const") ? static_cast<bool>(arg["const"]) : false).first;

					if (fullarg.empty() || !fullarg[0])
					{
						err = "Invalid Argument type found";
						return;
					}

					// Resolve invalid types for 'x86::Compiler::_newReg()' and insert type to the funcsig
					for (uint32_t& type : fullarg)
						m_FuncSig.addArg(resolve_type(type));
				}
			}
		}

		// Finally if it contains a variable argument, set it and ignore the |this| ptr, it should be passed on the stack
		if (const auto va_index = sig_data.find("va index"); va_index != sig_data.end() && va_index->is_number_integer())
			m_FuncSig.setVaIndex(static_cast<int>(*va_index) + (m_FuncSig.callConv() == CallConv::kIdThisCall ? 1 : 0));
	}


	std::unique_ptr<CallContext> SigBuilder::load_args(JIT::x86::Compiler& comp, TypeInfo& info)
	{
		using namespace JIT;

		const bool is_thiscall = m_FuncSig.callConv() == CallConv::kIdThisCall;
		// a hacky way to make va_args works in asmjit with thiscall convention
		// otherwise it will grab a garbage ecx register, and treat the actual thisptr pushed in the stack as an argument
		if (is_thiscall && m_FuncSig.hasVarArgs())
			m_FuncSig.setCallConv(CallConv::kIdHost);

		this->ManageFuncFrame(comp.addFunc(m_FuncSig)->frame());

		size_t arg_pos = 0;

		std::vector<std::pair<size_t, bool>> arg_buf;

		// if the convention is |this| call
		// set the first arg for compiler to IntPtr and insert a 'void*' type to 'arg_buf'
		if (is_thiscall)
		{
			info.m_ContainThisPtr = true;
			arg_buf.emplace_back(sizeof(void*), m_SigInfo.contains("mutable") ? m_SigInfo["mutable"].get<bool>() : true);
			comp.setArg(arg_pos++, info.m_Args.emplace_back(comp.newIntPtr(), sizeof(void*)).Reg);
		}


		// Set return in compiler
		{
			// if the return value is on the stack, set push the arg_pos and advance it in compiler
			if (m_RetTypes.size() > 1)
			{
				info.m_RetType = TypeInfo::RetType::RetMem;
				info.m_Ret[0] = { comp.newUInt32(), 4 };
				comp.setArg(arg_pos++, info.m_Ret[0].Reg);
			}
			else if (m_FuncSig.hasRet())
			{
				// check if we're in a 32bits env and if we should split the register
				if (const uint32_t type = m_RetTypes[0]; (Type::isInt64(type) || Type::isUInt64(type)) && comp.is32Bit())
				{
					info.m_RetType = TypeInfo::RetType::RetRegx2;
					for (auto& ret : info.m_Ret)
						ret = { comp.newReg(type - 2), 4 };
				}
				else
				{
					info.m_RetType = TypeInfo::RetType::RetReg;
					info.m_Ret[0] = { comp.newReg(type), Type::sizeOf(m_RetTypes[0]) };
				}
			}
			else
				info.m_RetType = TypeInfo::RetType::Void;
		}

		// Set args in compiler
		{
			for (auto& [types, is_const] : m_ArgTypes)
			{
				size_t underlying_size = m_Types.get_size(types);
				arg_buf.emplace_back(underlying_size, is_const);

				for (const uint32_t type : types)
				{
					// get type's true size, if it's [U]IntPtr, convert it to [U]Int32/[U]Int64
					size_t type_size = Type::sizeOf(type);

					if ((Type::isInt64(type) || Type::isUInt64(type)) && comp.is32Bit())
					{
						const auto& reg = info.m_Args.emplace_back(comp.newReg(type - 2), 4, comp.newReg(type - 2));

						comp.setArg(arg_pos, 0, reg.Reg);
						comp.setArg(arg_pos, 1, reg.ExtraReg);
					}
					else
					{
						const auto& reg = info.m_Args.emplace_back(comp.newReg(type), type_size);
						comp.setArg(arg_pos, reg.Reg);
					}

					++arg_pos;
				}
			}
		}

		CallContext::InitToken token{
			.FuncSig = m_FuncSig,
			.RetData = m_Types.get_size(m_RetTypes),
			.ArgsData{ std::move(arg_buf), is_thiscall }
		};

		return std::make_unique<CallContext>(std::move(token));
	}

	void SigBuilder::SetCallConv(const std::string& callconv)
	{
		using namespace JIT;
		
		std::unordered_map<std::string, uint32_t> callconvs{
			{ "Host",       CallConv::kIdHost },
			{ "CDecl",      CallConv::kIdCDecl },

			{ "STDCall",    CallConv::kIdStdCall },

			{ "FastCall",	CallConv::kIdFastCall },

			{ "VectorCall", CallConv::kIdVectorCall },

			{ "ThisCall",   CallConv::kIdThisCall },

			{ "RegParam1",  CallConv::kIdRegParm1},
			{ "RegParam2",  CallConv::kIdRegParm2 },
			{ "RegParam3",  CallConv::kIdRegParm3 },

			{ "SoftFloat",  CallConv::kIdSoftFloat },
			{ "HardFloat",  CallConv::kIdHardFloat },

			{ "LightCall2", CallConv::kIdLightCall2 },
			{ "LightCall3", CallConv::kIdLightCall3 },
			{ "LightCall4", CallConv::kIdLightCall4 },

			{ "x64SysV",    CallConv::kIdX64SystemV },
			{ "x64Win",     CallConv::kIdX64Windows },
		};

		m_FuncSig.setCallConv(callconvs[callconv]);
	}

	void SigBuilder::ManageFuncFrame(JIT::FuncFrame& func_frame) const
	{
		const auto flags = m_SigInfo.find("Flags");
		if (flags == m_SigInfo.end())
		{
			func_frame.setPreservedFP();
			return;
		}
		
		if (const auto fp = flags->find("no FP"); fp != flags->end() || !fp->get<bool>())
			func_frame.setPreservedFP();

		using FrameAttributes = JIT::FuncFrame::Attributes;

		const std::vector<std::pair<const char*, FrameAttributes>> frame_attributes
		{
			{ "func calls", FrameAttributes::kAttrHasFuncCalls },
			{ "AVX",		FrameAttributes::kAttrX86AvxEnabled },
			{ "AVX512",		FrameAttributes::kAttrX86Avx512Enabled },
			{ "MMX Clean",	FrameAttributes::kAttrX86MmxCleanup },
			{ "AVX Clean",	FrameAttributes::kAttrX86AvxCleanup },
		};

		for (auto& [name, attribute] : frame_attributes)
		{
			if (const auto attr = flags->find(name); attr != flags->end() && attr->get<bool>())
				func_frame.addAttributes(attribute);
		}
	}
}