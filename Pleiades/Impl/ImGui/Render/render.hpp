#pragma once

#include <map>
#include <functional>

#include <shadowgarden/interfaces/ImGui.hpp>
#include <shadowgarden/interfaces/InterfacesSys.hpp>
#include <shadowgarden/users/FontAwesome_Icons.hpp>

#include "Themes/Themes.hpp"

SG_NAMESPACE_BEGIN;

#define PlVersionFmt(ver)	ver.major(), ver.minor(), ver.build(), ver.revision()

class ImGui_BrdigeRenderer
{
public:
	using key_type = const char*;
	using callback_type = ImGuiPluginCallback;

	struct PropManager_t;
	struct CallbackState
	{
		callback_type Callback;
		IPlugin* Plugin;
		SG::ImGuiPlCallbackId Id;

		bool _Changed{ };
		bool _SafeToClose{ };

		void RenderInfo(PropManager_t& prop_manager);

		/// <summary>
		/// Send Popup for shutdown for saving/cancelling/discarding current data
		/// </summary>
		bool DisplayPopupShutown(key_type name, PropManager_t& prop_manager);
	};

	using container_type = std::multimap<key_type, CallbackState>;
	using container_iter = container_type::iterator;

	bool RenderAll();
	// Props Manager
	void RenderPropManager();
	// Plugins Manager
	void RenderPluginManager();
	// Logger
	void RenderLogger();
	// Profiler
	void RenderProfiler();
	// Console
	void RenderConsole();
	// About
	void RenderAbout();

public:
	void LoadTabs(uint32_t serialized_tabs);
	void SaveTabs(nlohmann::json& out_config);

	struct PropManager_t
	{ 
		container_type CallbackProps;
		container_iter Current{ CallbackProps.end() };
		ImGuiTextFilter Filter;

		/// <summary>
		/// Loop through 'CallbackProps', and collect the ones that aren't '_SafeToClose'
		/// </summary>
		container_iter GetFirstChanged();
	};

	PropManager_t PropManager;

	struct MainTabsInfo_t
	{
		using callback_t = void (ImGui_BrdigeRenderer::*)();

		const char* const Name;
		const callback_t Callback;

		bool IsOpen{ true };
		bool IsFocused{ false };

		MainTabsInfo_t(const char* name, callback_t callback) noexcept : Name(name), Callback(callback) { }
	};

	MainTabsInfo_t MainTabsInfo[5]{
		{ ICON_FA_STOPWATCH	" Profiler",		&ImGui_BrdigeRenderer::RenderProfiler },
		{ ICON_FA_BOOKMARK	" Logger",			&ImGui_BrdigeRenderer::RenderLogger }, 
		{ ICON_FA_ARCHIVE	" Props Manager",	&ImGui_BrdigeRenderer::RenderPropManager },
		{ ICON_FA_CLIPBOARD	" Plugins Manager",	&ImGui_BrdigeRenderer::RenderPluginManager },
		{ ICON_FA_TERMINAL	" Console",			&ImGui_BrdigeRenderer::RenderConsole }
	};

	ImGuiThemesManager ThemeManager;
};


SG_NAMESPACE_END;