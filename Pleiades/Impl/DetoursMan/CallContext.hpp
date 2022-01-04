#pragma once

#include <stack>
#include <asmjit/asmjit.h>
#include <px/interfaces/HookArgs.hpp>

#include "SigBuilder.hpp"


PX_NAMESPACE_BEGIN();

class CallContext
{
public:
	struct InitToken
	{
		asmjit::FuncSignatureBuilder FuncSig;
		PassRet	RetData;
		PassArgs ArgsData;
	};

	CallContext(InitToken&& token);

	/// <summary>
	/// Read/Write args from/to compiler in detoured function
	/// </summary>
	void ManageArgs(const DetourDetail::TypeInfo& typeInfo, asmjit::x86::Compiler& comp, bool read_from_compiler);

	/// <summary>
	/// Check for any changed args and write them to 'm_PassArgs.m_CurData'
	/// </summary>
	void WriteChangedArgs();

	/// <summary>
	/// Check for if we changed the return and write them to 'PassRet.m_CurData'
	/// </summary>
	void WriteChangedReturn();

	/// <summary>
	/// (Return in register)
	/// Read/Write return from/to compiler in the detoured function
	/// </summary>
	void ManageReturn(asmjit::x86::Compiler& comp, bool read_from_compiler, const asmjit::BaseReg& reg0, const asmjit::BaseReg& reg1);

	/// <summary>
	/// (Return in stack)
	/// Read/Write return from/to compiler in the detoured function
	/// </summary>
	void ManageReturnInMem(asmjit::x86::Compiler& comp, bool read_from_compiler, const asmjit::BaseReg& outreg);

	/// <summary>
	/// Save current return value in 'm_SavedRets'
	/// </summary>
	void SaveReturn();
	
	/// <summary>
	/// Load current return value from 'm_SavedRets' and pop it
	/// </summary>
	void RestoreReturn();

	/// <summary>
	/// Set 'PassRet.m_Changed' and 'PassArgs.m_ArgInfo[].m_Changed' to false
	/// </summary>
	void ResetState();

public:
	asmjit::FuncSignatureBuilder	m_FuncSig;

	PassRet	m_PassRet;
	PassArgs m_PassArgs;

private:
	std::stack<
		std::unique_ptr<PassArgs::data_arr>
	> m_SavedRets;
};
	
PX_NAMESPACE_END();