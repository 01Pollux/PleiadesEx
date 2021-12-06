#pragma once

#include "HookInstance.hpp"


SG_NAMESPACE_BEGIN;

class DetoursManager : public IDetoursManager
{
public:
	// Inherited via IDetoursManager
	IHookInstance* LoadHook(const std::vector<std::string>& keys, const char* hookName, void* pThis, IGameData* pPlugin, IntPtr lookupKey, IntPtr pAddr) override;

	void ReleaseHook(IHookInstance*& hookInst) override;

private:
	std::map<IntPtr, std::unique_ptr<IHookInstance>> m_ActiveHooks;
};

extern DetoursManager detour_manager;

SG_NAMESPACE_END;