
#include <fstream>

#include "Manager.hpp"
#include "library/Manager.hpp"

#include "library/Manager.hpp"		// lib_manager
#include "events/Manager.hpp"		// event_manager
#include "Logs/Logger.hpp"			// logger
#include "detours/HooksManager.hpp"	// detour_manager
#include "imgui/imgui_iface.hpp"	// imgui_iface
#include "console/Manager.hpp"		// console_manager


px::version DLLManager::GetHostVersion()
{
	constexpr px::version hostVersion{ "1.5.2.0" };
	return hostVersion;
}

bool DLLManager::BasicInit()
{
	namespace fs = std::filesystem;

	if (!fs::exists(LibraryManager::MainCfg))
		return false;

	if (fs::file_size(LibraryManager::MainCfg) < 2)
		return false;

	struct InterfaceAndName { px::IInterface* IFace; const char* Name; };

	for (const auto& info : {
			 InterfaceAndName{ &px::lib_manager,	px::Interface_ILibrary },
			 InterfaceAndName{ &px::logger,			px::Interface_ILogger },
			 InterfaceAndName{ &px::event_manager,	px::Interface_EventManager },
			 InterfaceAndName{ &px::detour_manager,	px::Interface_DetoursManager },
			 InterfaceAndName{ &px::imgui_iface,	px::Interface_ImGuiLoader },
			 InterfaceAndName{ &px::console_manager,px::Interface_ConsoleManager },
		})
	{
		// plugin is nullptr if its an internal interface, and shouldn't be removed at all cost
		ExposeInterface(info.Name, info.IFace, nullptr);
	}

	auto maincfg{ nlohmann::json::parse(std::ifstream(LibraryManager::MainCfg), nullptr, false, true) };
	if (maincfg.is_discarded())
		return false;

	{
		auto& name = maincfg["host name"];
		px::lib_manager.SetHostName(name.is_string() ? name : "any");
	}

	if (!px::imgui_iface.LoadImGui(maincfg))
		return false;

	px::console_manager.AddCommands();

	auto plugins = maincfg.find("plugins");
	if (plugins != maincfg.end() && plugins->is_array() && !plugins->empty())
		std::thread([](nlohmann::json&& plugins) { px::plugin_manager.LoadAllDLLs(plugins); }, std::move(*plugins)).detach();

	return true;
}

void DLLManager::BasicShutdown()
{
	auto maincfg{ nlohmann::json::parse(std::ifstream(LibraryManager::MainCfg)) };
	px::imgui_iface.SaveImGui(maincfg);
	px::imgui_iface.UnloadImGui();

	std::ofstream file(LibraryManager::MainCfg);
	file.width(2);
	file << maincfg;
}

bool DLLManager::ExposeInterface(const std::string& iface_name, px::IInterface* iface, px::IPlugin* owner)
{
	auto iter = m_Interfaces.find(iface_name);
	if (iter != m_Interfaces.end())
		return false;

	m_Interfaces.emplace(iface_name, InterfaceInfo{ iface, owner });
	return true;
}

bool DLLManager::RequestInterface(const std::string& iface_name, px::IInterface** iface)
{
	auto iter = m_Interfaces.find(iface_name);
	if (iter == m_Interfaces.end())
		return false;

	*iface = iter->second.IFace;
	return true;
}
