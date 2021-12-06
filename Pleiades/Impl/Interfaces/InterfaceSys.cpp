#include <fstream>

#include "Impl/Plugins/PluginManager.hpp"
#include "Impl/Library/LibrarySys.hpp"		// lib_manager
#include "Impl/Interfaces/EventManager.hpp"	// event_manager
#include "Impl/Interfaces/Logger.hpp"		// logger
#include "Impl/DetoursMan/HooksManager.hpp"	// detour_manager
#include "Impl/ImGui/imgui_iface.hpp"		// imgui_iface


SG_NAMESPACE_BEGIN;

Version DLLManager::GetHostVersion()
{
	constexpr Version hostVersion{ "1.25.0.0" };
	return hostVersion;
}

bool DLLManager::BasicInit()
{
	namespace fs = std::filesystem;

	if (!fs::exists(LibraryManager::MainCfg))
		return false;

	if (fs::file_size(LibraryManager::MainCfg) < 2)
		return false;

	struct InterfaceAndName { IInterface* IFace; const char* Name; };

	for (const auto& info : {
			 InterfaceAndName{ &lib_manager,	Interface_ILibrary },
			 InterfaceAndName{ &logger,			Interface_ILogger },
			 InterfaceAndName{ &event_manager,	Interface_EventManager },
			 InterfaceAndName{ &detour_manager,	Interface_DetoursManager },
			 InterfaceAndName{ &imgui_iface,	Interface_ImGuiLoader },
	})
	{
		// plugin is nullptr if its an internal interface, and shouldn't be removed at all cost
		ExposeInterface(info.Name, info.IFace, nullptr);
	}


	Json maincfg = Json::parse(std::ifstream(LibraryManager::MainCfg));
	{
		auto& name = maincfg["host name"];
		SG::lib_manager.SetHostName(name.is_string() ? name : "any");
	}

	if (!SG::imgui_iface.LoadImGui(maincfg))
		return false;

	auto plugins = maincfg.find("plugins");
	if (plugins != maincfg.end() && plugins->is_array() && !plugins->empty())
		std::thread([](Json&& plugins) { SG::plugin_manager.LoadAllDLLs(plugins); }, std::move(*plugins)).detach();

	return true;
}

bool DLLManager::ExposeInterface(const char* iface_name, IInterface* iface, IPlugin* owner)
{
	std::string key{ iface_name };

	auto iter = m_Interfaces.find(key);
	if (iter != m_Interfaces.end())
		return false;

	m_Interfaces.emplace(key, InterfaceInfo{ iface, owner });
	return true;
}

bool DLLManager::RequestInterface(const std::string& iface_name, IInterface** iface)
{
	auto iter = m_Interfaces.find(iface_name);
	if (iter == m_Interfaces.end())
		return false;

	*iface = iter->second.IFace;
	return true;
}

SG_NAMESPACE_END;