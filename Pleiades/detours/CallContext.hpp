#pragma once

#include <stack>
#include <asmjit/asmjit.h>
#include <px/interfaces/HookArgs.hpp>

#include "SigBuilder.hpp"

class DetourCallContext
{
public:
	struct InitToken
	{
		asmjit::FuncSignatureBuilder FuncSig;
		px::PassRet	RetData;
		px::PassArgs ArgsData;
	};

	DetourCallContext(InitToken&& token) noexcept;

	/// <summary>
	/// Read/Write args from/to compiler in detoured function
	/// </summary>
	void ManageArgs(const detour_detail::TypeInfo& typeInfo, asmjit::x86::Compiler& comp, bool read_from_compiler);

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
	/// Save current argss to 'm_PassArgs'
	/// </summary>
	void PushArgs();
	
	/// <summary>
	/// load args from 'm_PassArgs' and pop it
	/// </summary>
	void PopArgs();

	/// <summary>
	/// Save current return value to 'm_SavedRets'
	/// </summary>
	void PushReturn();
	
	/// <summary>
	/// Load current return value from 'm_SavedRets' and pop it
	/// </summary>
	void PopReturn();

	/// <summary>
	/// Set 'PassRet.m_Changed' and 'PassArgs.m_ArgInfo[].m_Changed' to false
	/// </summary>
	void ResetState();

public:
	asmjit::FuncSignatureBuilder	m_FuncSig;

	px::PassRet	m_PassRet;
	px::PassArgs m_PassArgs;

private:
	std::vector<
		std::unique_ptr<px::PassArgs::data_arr>
	> m_SavedRets,
	  m_SavedArgs;
};
