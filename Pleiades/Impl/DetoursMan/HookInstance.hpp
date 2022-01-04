#pragma once

#include <set>
#include <nlohmann/Json_Fwd.hpp>

#include <px/interfaces/HooksManager.hpp>
#include <px/intptr.hpp>

#include "Detour.hpp"
#include "CallContext.hpp"


PX_NAMESPACE_BEGIN();

struct HookInfo
{
	IHookInstance::CallbackType	Callback;
	HookOrder					Order;
	IHookInstance::HookID		Id;

	auto operator<=>(const HookInfo& o) const noexcept { return Order <=> o.Order; }
	bool operator==(const IHookInstance::HookID& o) const noexcept { return Id == o; }
};

class HookInstance : public IHookInstance
{
public:
	HookInstance(IntPtr original_function, const nlohmann::json& data, std::string& out_err);
	~HookInstance() noexcept;

public:
	// Inherited via IHookInstance
	HookID AddCallback(bool post, HookOrder order, const CallbackType& callback) override;

	void RemoveCallback(bool post, HookID id) noexcept override;

	MHookRes GetLastResults() noexcept override;

	void* GetFunction() noexcept override;

	size_t RefCount{ 1 };
	IntPtr m_AddressInMemory;

private:
	struct DataInfo
	{
		const DetourDetail::TypeInfo& typeInfo;
		asmjit::x86::Compiler& Compiler;
	};

	void* AllocCallbackHandler(DetourDetail::SigBuilder& sigbuilder, std::string& out_err);

	bool ValidateRegisters(DataInfo comp, std::string& out_err);
	bool RunHandler(bool is_post);

	void InvokeCallbacks(DataInfo info, bool post, const asmjit::x86::Gp& ret);
	void InvokeOriginal(DataInfo info);

	void ReadReturn(DataInfo info);
	void WriteReturn(DataInfo info);

	std::unique_ptr<CallContext> m_CallContext;

	size_t m_ReferenceCount{ };
	std::multiset<HookInfo>
		m_PreCallbacks,
		m_PostCallbacks;

	Detour m_Detour;
	void* m_ActualFunction{ };

	std::stack<MHookRes>	m_LastResults;
	std::stack<MHookRes>	m_LastReturn;
};

PX_NAMESPACE_END();