#pragma once

#include "HookInstance.hpp"

class DetoursManager : public px::IDetoursManager
{
public:
	// Inherited via IDetoursManager
	px::IHookInstance* LoadHook(
		const std::vector<std::string>& keys, 
		const char* hookName, 
		px::IntPtr pThis,
		px::IGameData* pPlugin,
		px::IntPtr lookupKey, 
		px::IntPtr pAddr
	) override;

	void ReleaseHook(px::IHookInstance*& hookInst) override;

	void SleepToReleaseHooks();

private:
	std::map<px::IntPtr, std::unique_ptr<px::IHookInstance>> m_ActiveHooks;
	std::map<px::IntPtr, std::unique_ptr<px::IHookInstance>> m_FreeHooks;
};

PX_NAMESPACE_BEGIN();
inline DetoursManager detour_manager;
PX_NAMESPACE_END();