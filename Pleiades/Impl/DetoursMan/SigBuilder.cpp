#include <fstream>

#include "SigBuilder.hpp"
#include "CallContext.hpp"

#include "Impl/Interfaces/Logger.hpp"


namespace px::DetourDetail
{
	SigBuilder::SigBuilder(const nlohmann::json& sig_data, std::string& err) :
		m_SigInfo(sig_data)
	{
		using namespace asmjit;
		// return value = the value to be inserted in funcsig
		// &type = the value to be inserted with 'x86::Compiler::_newReg()' in the future
		//
		// the function will be used later on for resolving 'm_RetTypes' and 'm_ArgTypes'
		const auto resolve_type =
			[] (TypeId& type) -> TypeId
		{
			// ints, only deabstract [U]InPtr
			if (type <= TypeId::_kIntEnd)
			{
				type = TypeUtils::deabstract(type, TypeUtils::deabstractDeltaOfSize(sizeof(void*)));
				return type;
			}

			switch (type)
			{
			case TypeId::kFloat32:
			case TypeId::kFloat32x1:
			{
				type = TypeId::kFloat32x1;
				return TypeId::kFloat32;
			}
			case TypeId::kFloat64:
			case TypeId::kFloat64x1:
			{
				type = TypeId::kFloat64x1;
				return TypeId::kFloat64;
			}
			case TypeId::kFloat80:
			{
				return TypeId::kFloat80;
			}

			default:
			{
				if (type >= TypeId::_kMmxStart && type <= TypeId::_kMmxEnd)
				{
					return static_cast<TypeId>(TypeUtils::TypeIdOfT<x86::Mm>::kTypeId);
				}
				else if (type >= TypeId::_kVec32Start && type <= TypeId::_kVec64End)
				{
					return type;
				}
				else if (type >= TypeId::_kVec128Start && type <= TypeId::_kVec128End)
				{
					return static_cast<TypeId>(TypeUtils::TypeIdOfT<x86::Xmm>::kTypeId);
				}
				else if (type >= TypeId::_kVec256Start && type <= TypeId::_kVec256Start)
				{
					return static_cast<TypeId>(TypeUtils::TypeIdOfT<x86::Ymm>::kTypeId);
				}
				else if (type >= TypeId::_kVec512Start && type <= TypeId::_kVec512End)
				{
					return static_cast<TypeId>(TypeUtils::TypeIdOfT<x86::Zmm>::kTypeId);
				}
				else
					// !!!
					return static_cast<TypeId>(0);
			}
			}
		};

		if (sig_data.empty())
		{
			err = "Detour Entry is empty.";
			return;
		}

		SetCallConv(sig_data["callConv"].get_ref<const std::string&>());
		if (m_FuncSig.callConvId() == CallConvId::kNone)
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
			TypeId final_ret_sig{ };
			for (TypeId& type : m_RetTypes)
			{
				final_ret_sig = resolve_type(type);
			}

			// if the type's size is bigger than 1 type, it should return in the stack, else we will use m_RetType[0] (either void, int, etc)
			if (m_RetTypes.size() > 1)
			{
				m_FuncSig.setRetT<void>();
				m_FuncSig.addArgT<void*>();
			}
			else
			{
				m_FuncSig.setRet(static_cast<TypeId>(final_ret_sig));
			}
		}

		// Read the args types
		{
			// if the callconv is thiscall, use void* for |this| pointer first param
			if (m_FuncSig.callConvId() == CallConvId::kThisCall)
				m_FuncSig.addArg(TypeId::kUIntPtr);

			if (const auto args = sig_data.find("Arguments"); args != sig_data.end())
			{
				m_ArgTypes.reserve(args->size());
				for (auto& arg : *args)
				{
					auto& fullarg = m_ArgTypes.emplace_back(m_Types.load_type(arg["type"]), arg.contains("const") ? static_cast<bool>(arg["const"]) : false).first;

					if (fullarg.empty() || fullarg[0] == TypeId::kVoid)
					{
						err = "Invalid Argument type found";
						return;
					}

					// Resolve invalid types for 'x86::Compiler::_newReg()' and insert type to the funcsig
					for (TypeId& type : fullarg)
						m_FuncSig.addArg(resolve_type(type));
				}
			}
		}

		// Finally if it contains a variable argument, set it and ignore the |this| ptr, it should be passed on the stack
		if (const auto va_index = sig_data.find("va index"); va_index != sig_data.end() && va_index->is_number_integer())
			m_FuncSig.setVaIndex(static_cast<int>(*va_index) + (m_FuncSig.callConvId() == CallConvId::kThisCall ? 1 : 0));
	}


	std::unique_ptr<CallContext> SigBuilder::load_args(asmjit::x86::Compiler& comp, TypeInfo& info)
	{
		using namespace asmjit;

		const bool is_thiscall = m_FuncSig.callConvId() == CallConvId::kThisCall;
		// a hacky way to make va_args works in asmjit with thiscall convention
		// otherwise it will grab a garbage ecx register, and treat the actual thisptr pushed in the stack as an argument
		if (is_thiscall && m_FuncSig.hasVarArgs())
			m_FuncSig.setCallConvId(CallConvId::kHost);

		FuncNode* pFunc = comp.addFunc(m_FuncSig);
		this->ManageFuncFrame(pFunc->frame());

		size_t arg_pos = 0;

		std::vector<std::pair<size_t, bool>> arg_buf;

		// if the convention is |this| call
		// set the first arg for compiler to IntPtr and insert a 'void*' type to 'arg_buf'
		if (is_thiscall)
		{
			info.m_ContainThisPtr = true;
			arg_buf.emplace_back(sizeof(void*), m_SigInfo.contains("mutable") ? m_SigInfo["mutable"].get<bool>() : true);
			pFunc->setArg(arg_pos++, info.m_Args.emplace_back(comp.newIntPtr(), sizeof(void*)).Reg);
		}


		// Set return in compiler
		{
			// if the return value is on the stack, set push the arg_pos and advance it in compiler
			if (m_RetTypes.size() > 1)
			{
				info.m_RetType = TypeInfo::RetType::RetMem;
				info.m_Ret[0] = { comp.newUInt32(), 4 };
				pFunc->setArg(arg_pos++, info.m_Ret[0].Reg);
			}
			else if (m_FuncSig.hasRet())
			{
				// check if we're in a 32bits env and if we should split the register
				if (const TypeId type = m_RetTypes[0]; (TypeUtils::isInt64(type) || TypeUtils::isUInt64(type)) && comp.is32Bit())
				{
					info.m_RetType = TypeInfo::RetType::RetRegx2;
					for (auto& ret : info.m_Ret)
						ret = { comp.newReg(static_cast<TypeId>(static_cast<uint32_t>(type) - 2)), 4 /* sizeof(eax) */ };
				}
				else
				{
					info.m_RetType = TypeInfo::RetType::RetReg;
					info.m_Ret[0] = { comp.newReg(type), TypeUtils::sizeOf(m_RetTypes[0]) };
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

				for (const TypeId type : types)
				{
					// get type's true size, if it's [U]IntPtr, convert it to [U]Int32/[U]Int64
					size_t type_size = TypeUtils::sizeOf(type);

					if ((TypeUtils::isInt64(type) || TypeUtils::isUInt64(type)) && comp.is32Bit())
					{
						const auto& reg = info.m_Args.emplace_back(
							comp.newReg(static_cast<TypeId>(static_cast<uint32_t>(type))), 
							4 /* sizeof(eax) */,
							comp.newReg(static_cast<TypeId>(static_cast<uint32_t>(type)))
						);

						pFunc->setArg(arg_pos, 0, reg.Reg);
						pFunc->setArg(arg_pos, 1, reg.ExtraReg);
					}
					else
					{
						const auto& reg = info.m_Args.emplace_back(comp.newReg(type), type_size);
						pFunc->setArg(arg_pos, reg.Reg);
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
		using namespace asmjit;
		
		std::unordered_map<std::string, CallConvId> callconvs{
			{ "Host",       CallConvId::kHost },
			{ "CDecl",      CallConvId::kCDecl },
									
			{ "STDCall",    CallConvId::kStdCall },
									
			{ "FastCall",	CallConvId::kFastCall },
									
			{ "VectorCall", CallConvId::kVectorCall },
									
			{ "ThisCall",   CallConvId::kThisCall },
									
			{ "RegParam1",  CallConvId::kRegParm1},
			{ "RegParam2",  CallConvId::kRegParm2 },
			{ "RegParam3",  CallConvId::kRegParm3 },
									
			{ "SoftFloat",  CallConvId::kSoftFloat },
			{ "HardFloat",  CallConvId::kHardFloat },
									
			{ "LightCall2", CallConvId::kLightCall2 },
			{ "LightCall3", CallConvId::kLightCall3 },
			{ "LightCall4", CallConvId::kLightCall4 },
									
			{ "x64SysV",    CallConvId::kX64SystemV },
			{ "x64Win",     CallConvId::kX64Windows },
		};

		m_FuncSig.setCallConvId(callconvs[callconv]);
	}

	void SigBuilder::ManageFuncFrame(asmjit::FuncFrame& func_frame) const
	{
		const auto flags = m_SigInfo.find("Flags");
		if (flags == m_SigInfo.end())
		{
			func_frame.setPreservedFP();
			return;
		}
		
		if (const auto fp = flags->find("no FP"); fp != flags->end() || !fp->get<bool>())
			func_frame.setPreservedFP();

		using FrameAttributes = asmjit::FuncAttributes;

		const std::vector<std::pair<const char*, FrameAttributes>> frame_attributes
		{
			{ "func calls", FrameAttributes::kHasFuncCalls },
			{ "AVX",		FrameAttributes::kX86_AVXEnabled },
			{ "AVX512",		FrameAttributes::kX86_AVX512Enabled },
			{ "AVX Clean",	FrameAttributes::kX86_AVXCleanup },
			{ "MMX Clean",	FrameAttributes::kX86_MMXCleanup },
		};

		for (auto& [name, attribute] : frame_attributes)
		{
			if (const auto attr = flags->find(name); attr != flags->end() && attr->get<bool>())
				func_frame.addAttributes(attribute);
		}
	}
}