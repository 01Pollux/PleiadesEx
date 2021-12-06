#include "Defines.hpp"
#include "Interfaces/InterfacesSys.hpp"
#include "Interfaces/GameData.hpp"

#include "Funcs.hpp"
#include "Tests/TestCase.hpp"

#include <boost/system.hpp>
#ifdef BOOST_WINDOWS
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif

class FuncHooksTest : public SG::IPluginImpl
{
public:
	bool OnPluginLoad2(SG::IPluginManager* ifacemgr) override;
	
	void OnPluginUnload() override;

public:
#ifdef BOOST_WINDOWS
	FILE* hSTDOut{ };
#endif
};

// Important for exporting plugin
SG_DLL_EXPORT(FuncHooksTest);


/**
* There are three ways to hook a function and share it accross single/multiple dlls
*
* First one is to rely on signatures, address keys in gamedata
* IDetoursManager::LoadHook's 'pThis', 'lookupKey' and 'pAddr' should be dismissed/nullptr
*
* Second one is to rely on virtual
* IDetoursManager::LoadHook's 'lookupKey' and 'pAddr' should be dismissed/nullptr
* while 'pThis' must not be nullptr. note: LoadHook() will always start by checking if the signature exists, if not, then the function will fail
*
* Third way is to dynamically hook function from a function that can be obtained only in runtime
* IDetoursManager::LoadHook's 'lookupKey' and 'pAddr' mustn't be dismissed/nullptr
* 'lookupKey' can be from something unique like address of function, address of address struct pointing to the function
* 'pAddr' should be the function target to be detoured
*
* struct SomeStruct
* {
* 	void* pData;
* 	void* pCallback;
* 	//...
* };
* //SomeStruct* pInfo;
* SG::IntPtr pKey = &pInfo->pCallback;
* SG::IntPtr pAddr = pInfo->pCallback;
*
*
* In this example, we'll be using function's address since we can obtain it before dll's execution and it's way easier to maintain if we made a code change
*/
bool FuncHooksTest::OnPluginLoad2(SG::IPluginManager* ifacemgr)
{
#ifdef BOOST_WINDOWS
	if (!AllocConsole())
		return false;
	freopen_s(&hSTDOut, "CONOUT$", "a", stdout);
#endif

	SG_PROFILE_SECTION("PluginLoad", __FUNCTION__);

	std::unique_ptr<SG::IGameData> pData(SG::LibManager->OpenGameData(this));
	pData->PushFiles({ "/GameData/ThisCall" });

	SG::ITestCase::Register(pData.get());
	
	ImGui::SetCurrentContext(SG::ImGuiLoader->GetContext());
	SG::ImGuiLoader->AddCallback(
		this,
		"Function Hook Tests",
		[this] () -> bool
		{
			SG::ITestCase::Render();
			return false;
		}
	);

	return true;
}

void FuncHooksTest::OnPluginUnload()
{
	SG::ITestCase::Unregister();

#if BOOST_WINDOWS
	fclose(hSTDOut);
	FreeConsole();
#endif
}

