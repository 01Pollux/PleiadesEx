#pragma once

#include <set>
#include <mutex>
#include <nlohmann/json_fwd.hpp>

#include <px/interfaces/HooksManager.hpp>
#include <px/intptr.hpp>

#include "Detour.hpp"
#include "CallContext.hpp"


struct HookInfo
{
	px::IHookInstance::CallbackType	Callback;
	px::HookOrder					Order;
	px::IHookInstance::HookID		Id;

	auto operator<=>(const HookInfo& o) const noexcept { return Order <=> o.Order; }
	bool operator==(const px::IHookInstance::HookID& o) const noexcept { return Id == o; }
};

class HookInstance : public px::IHookInstance
{
public:
	HookInstance(px::IntPtr original_function, const nlohmann::json& data, std::string& out_err);
	~HookInstance() noexcept {}

	void ClearCallbacks() noexcept;

	void Activate();
	void Deactivate();

	void* GetRuntimeCallback();

public:
	// Inherited via IHookInstance
	HookID AddCallback(bool post, px::HookOrder order, const CallbackType& callback) override;

	void RemoveCallback(bool post, HookID id) override;

	px::MHookRes GetLastResults() noexcept override;

	void* GetFunction() noexcept override;

	void* GetStackPointer() noexcept override;

	size_t RefCount{ 1 };
	px::IntPtr m_AddressInMemory;

private:
	struct DataInfo
	{
		const detour_detail::TypeInfo& typeInfo;
		asmjit::x86::Compiler& Compiler;
	};

	[[nodiscard]] void* AllocCallbackHandler(detour_detail::SigBuilder& sigbuilder, std::string& out_err);

	[[nodiscard]] bool ValidateRegisters(DataInfo comp, std::string& out_err);
	[[nodiscard]] bool RunHandler(bool is_post);

	void InvokeCallbacks(DataInfo info, bool post, const asmjit::x86::Gp& ret);
	void InvokeOriginal(DataInfo info);

	void ReadReturn(DataInfo info);
	void WriteReturn(DataInfo info);

	std::unique_ptr<DetourCallContext> m_CallContext;
	std::multiset<HookInfo>
		m_PreCallbacks,
		m_PostCallbacks;

	void* m_StackPointer{ };
	std::mutex m_CallbacksLock;

	std::stack<px::MHookRes>	m_LastResults;
	detour_detail::Detour m_Detour;
};
