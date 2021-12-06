#include "HooksManager.hpp"

#include "Interfaces/PluginSys.hpp"
#include "Impl/Library/GameData.hpp"
#include "Impl/Interfaces/Logger.hpp"


SG_NAMESPACE_BEGIN;

DetoursManager detour_manager;

IHookInstance* DetoursManager::LoadHook(const std::vector<std::string>& keys, const char* hookName, void* pThis, IGameData* gamedata, IntPtr lookupKey, IntPtr pAddr)
{
	GameData* pData{ static_cast<GameData*>(gamedata) };
	Json res = pData->ReadDetour(keys, hookName);

	if (res.empty())
	{
		SG_LOG_ERROR(
			SG_MESSAGE("Tried loading an non-existing detour."),
			SG_LOGARG("Detour", hookName)
		);
		return nullptr;
	}

	if (!lookupKey && !pAddr)
	{
		enum class SecType
		{
			Virtual, Address, Signature
		};

		const auto get_addr_from_cfg = [&pData] (SecType sec_type, Json& cfg, void* pThis = nullptr) -> IntPtr
		{
			const char* sec_name =
				sec_type == SecType::Virtual ? "virtual" :
				sec_type == SecType::Address ? "address" :
				sec_type == SecType::Signature ? "name" : nullptr;

			Json& info = cfg[sec_name];
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

		if (Json& sig_sec = res["Signature"], &sig_name = sig_sec["name"];
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
		SG_LOG_ERROR(
			SG_MESSAGE("Failed to get address of detour."),
			SG_LOGARG("Detour", hookName)
		);
		return nullptr;
	}

	auto& hookInst = m_ActiveHooks[lookupKey];

	if (hookInst)
	{
		auto pInst = static_cast<HookInstance*>(hookInst.get());
		++pInst->RefCount;
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
		SG_LOG_ERROR(
			SG_MESSAGE("Exception reported while loading hook."),
			SG_LOGARG("Detour", hookName),
			SG_LOGARG("Exception", ex.what())
		);
	}

	return hookInst.get();
}

void DetoursManager::ReleaseHook(IHookInstance*& hookInst)
{
	HookInstance* pInst = static_cast<HookInstance*>(hookInst);
	--pInst->RefCount;
	hookInst = nullptr;

	if (!pInst->RefCount)
		m_ActiveHooks.erase(pInst->m_AddressInMemory);
}

SG_NAMESPACE_END;