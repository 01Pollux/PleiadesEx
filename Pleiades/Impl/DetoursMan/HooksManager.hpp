#pragma once

#include "HookInstance.hpp"


PX_NAMESPACE_BEGIN();

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

PX_NAMESPACE_END();