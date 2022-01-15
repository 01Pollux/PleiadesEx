#include "HooksManager.hpp"

#include <px/interfaces/PluginSys.hpp>

#include "library/Manager.hpp"
#include "plugins/GameData.hpp"
#include "Logs/Logger.hpp"

px::IHookInstance* DetoursManager::LoadHook(
	const std::vector<std::string>& keys,
	const char* hookName, 
	px::IntPtr pThis, 
	px::IGameData* gamedata, 
	px::IntPtr lookupKey,
	px::IntPtr pAddr
)
{
	GameData* pData{ static_cast<GameData*>(gamedata) };
	auto res = pData->ReadDetour(keys, hookName);

	if (res.empty())
	{
		PX_LOG_ERROR(
			PX_MESSAGE("Tried loading an non-existing detour."),
			PX_LOGARG("Detour", hookName)
		);
		return nullptr;
	}

	if (!lookupKey && !pAddr)
	{
		enum class SecType
		{
			Virtual, Address, Signature
		};

		const auto get_addr_from_cfg = [&pData] (SecType sec_type, nlohmann::json& cfg, void* pThis = nullptr) -> px::IntPtr
		{
			const char* sec_name =
				sec_type == SecType::Virtual ? "virtual" :
				sec_type == SecType::Address ? "address" :
				sec_type == SecType::Signature ? "name" : nullptr;

			auto& info = cfg[sec_name];
			if (info.is_array())
			{
				// "sec_name": [ "Main", "Class", "Subclass", "Sub-subclass", "FuncName" ]
				switch (sec_type)
				{
				case SecType::Virtual:
					return pData->ReadVirtual(info, { }, pThis);

				case SecType::Address:
					return pData->ReadAddress(info, { });

				case SecType::Signature:
					return pData->ReadSignature(info, { });

				default: break;
				}
			}
			else if (info.is_string())
			{
				// "sec_name": "Main::Class::FuncName"
				switch (sec_type)
				{
				case SecType::Virtual:
					return pData->ReadVirtual({ }, info, pThis);

				case SecType::Address:
					return pData->ReadAddress({ }, info);

				case SecType::Signature:
					return pData->ReadSignature({ }, info);

				default: break;
				}
			}

			return nullptr;
		};

		if (auto& sig_sec = res["Signature"], &sig_name = sig_sec["name"];
			sig_name.empty())
		{
			if (pThis)
				pAddr = get_addr_from_cfg(SecType::Virtual, sig_sec, pThis);

			if (!pAddr)
				pAddr = get_addr_from_cfg(SecType::Address, sig_sec);
		}
		else
			pAddr = get_addr_from_cfg(SecType::Signature, sig_sec);
		lookupKey = pAddr;
	}

	if (!pAddr)
	{
		PX_LOG_ERROR(
			PX_MESSAGE("Failed to get address of detour."),
			PX_LOGARG("Detour", hookName)
		);
		return nullptr;
	}

	auto& hookInst = m_ActiveHooks[lookupKey];

	if (hookInst)
	{
		auto pInst = static_cast<HookInstance*>(hookInst.get());
		if (!pInst->RefCount++)
			pInst->Activate();
		return pInst;
	}

	try
	{
		std::string err;
		hookInst = std::make_unique<HookInstance>(pAddr, res, err);
		if (!err.empty())
			throw std::runtime_error(err);
	}
	catch (const std::exception& ex)
	{
		hookInst = nullptr;
		PX_LOG_ERROR(
			PX_MESSAGE("Exception reported while loading hook."),
			PX_LOGARG("Detour", hookName),
			PX_LOGARG("Exception", ex.what())
		);
	}

	return hookInst.get();
}

void DetoursManager::ReleaseHook(px::IHookInstance*& hookInst)
{
	if (hookInst)
	{
		HookInstance* pInst = static_cast<HookInstance*>(hookInst);
		hookInst = nullptr;

		if (!--pInst->RefCount)
			pInst->Deactivate();
	}
}

void DetoursManager::SleepToReleaseHooks()
{
	// Deactivate all the of the active hooks
	for (auto& hook : m_ActiveHooks)
	{
		HookInstance* pInst = static_cast<HookInstance*>(hook.second.get());
		if (pInst->RefCount)
			pInst->Deactivate();
	}

	// Sleep for a short time in case there is a function that hasnt finished its execution
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(10us);
	
	// Release all of the active hooks from JIT
	for (auto& hook : m_ActiveHooks)
	{
		HookInstance* pInst = static_cast<HookInstance*>(hook.second.get());
		px::lib_manager.GetRuntime()->release(pInst->GetRuntimeCallback());
	}

	// Free all of the hook pointers
	m_ActiveHooks.clear();
}
