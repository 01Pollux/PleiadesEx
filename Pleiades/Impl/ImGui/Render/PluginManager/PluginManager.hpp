#pragma once

#include "../render.hpp"
#include "../../imgui_iface.hpp"
#include "Impl/Plugins/PluginManager.hpp"


// TODO:
// Read Profiler

// PROFILER:
// CPU usage
// Callstack
// Diagram
SG_NAMESPACE_BEGIN;


enum class PluginState : char8_t
{
	Paused,
	Loaded,
	Unloaded,
	Failed
};

struct ImGuiPlInfo
{
	std::string PluginName;
	IPlugin*	Plugin{ };
	nlohmann::json	Logs;
	PluginState State{ PluginState::Unloaded };

	void Load();
	void Unload();
	void Reload();
	void PauseOrResume();

	void DrawPopupState();
	void DrawPluginsProps();
};

// For Plugin Manager
struct ImGuiPlManSection
{
	using container_type = std::vector<ImGuiPlInfo>;
	using iterator_type = container_type::iterator;

	container_type Plugins;
	ImGuiTextFilter Filter;
	iterator_type Current{ Plugins.end() };

	void reset() noexcept
	{
		Current = Plugins.end();
	}
};



class ImGui_PluginManager
{
public:
	ImGui_PluginManager(const std::string_view& host_name) :
		HostName(host_name),
		HostVer(SG::plugin_manager.GetHostVersion())
	{
		LoadPlugins();
	}

	/// <summary>
	/// Clear 'm_PlSection.Plugins' and re-insert plugins if they exists
	/// </summary>
	void LoadPlugins();

	/// <summary>
	/// This should setup our filter, plugin's information and call OnDrawPluginInfo for us to render extra informations
	/// </summary>
	void DrawPluginDesign();

	const std::string_view HostName;
	const Version HostVer;

private:
	ImGuiPlManSection m_PlManSection;
};


constexpr const char* PlStateToString(PluginState state)
{
	using enum PluginState;
	switch (state)
	{
	case Paused: return "Paused";
	case Loaded: return "Loaded";
	case Unloaded: return "Unloaded";
	case Failed: return "Failed";
	default: return "";
	}
}

SG_NAMESPACE_END;