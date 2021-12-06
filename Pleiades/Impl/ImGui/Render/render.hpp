#pragma once

#include <map>
#include <functional>
#include "Interfaces/ImGui.hpp"
#include "Interfaces/InterfacesSys.hpp"
#include "User/FontAwesome_Icons.hpp"
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
	// About
	void RenderAbout();


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
		const char* const Name;
		bool isOpen{ true };
		MainTabsInfo_t(const char* name) noexcept : Name(name) { }

		enum class Type
		{
			PropManager,
			PluginsManager,
			Logger,
			Profiler
		};
	};

	MainTabsInfo_t MainTabsInfo[4]{
		ICON_FA_ARCHIVE		" Props Manager",
		ICON_FA_CLIPBOARD	" Plugins Manager",
		ICON_FA_BOOKMARK	" Logger", 
		ICON_FA_STOPWATCH	" Profiler"
	};

	MainTabsInfo_t& GetTab(MainTabsInfo_t::Type type) noexcept
	{
		return MainTabsInfo[static_cast<size_t>(type)];
	}

	ImGuiThemesManager ThemeManager;
};


SG_NAMESPACE_END;