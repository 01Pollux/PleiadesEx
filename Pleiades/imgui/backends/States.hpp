#pragma once

#include <functional>

#include <px/interfaces/ImGui.hpp>
#include <px/interfaces/InterfacesSys.hpp>
#include <px/icons/FontAwesome.hpp>

#include "../frontends/themes/Themes.hpp"
#include "../imgui_iface.hpp"

#define PlVersionFmt(ver)	ver.major(), ver.minor(), ver.build(), ver.revision()

namespace renderer
{
	struct global_state
	{
		class ImGui_BrdigeRenderer
		{
		public:
			using key_type = const char*;
			using callback_type = px::ImGuiPluginCallback;

			struct PropManager_t;
			struct CallbackState
			{
				callback_type Callback;
				px::IPlugin* Plugin;
				px::ImGuiPlCallbackId Id;

				bool _Changed{ };
				bool _SafeToClose{ };

				void RenderInfo(PropManager_t& prop_manager);

				/// <summary>
				/// Send Popup for shutdown for saving/cancelling/discarding current data
				/// </summary>
				[[nodiscard]] bool DisplayPopupShutown(key_type name, PropManager_t& prop_manager);
			};

			using container_type = std::multimap<key_type, CallbackState>;
			using container_iter = container_type::iterator;

			void RenderAll();
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
				/// Loop through 'CallbackProps', find the first one that has changed
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

				MainTabsInfo_t(const char* name, callback_t callback) noexcept : Name(name), Callback(callback) {}
			};

			MainTabsInfo_t MainTabsInfo[5]{
				{ ICON_FA_STOPWATCH	" Profiler",		&ImGui_BrdigeRenderer::RenderProfiler },
				{ ICON_FA_BOOKMARK	" Logger",			&ImGui_BrdigeRenderer::RenderLogger },
				{ ICON_FA_ARCHIVE	" Props Manager",	&ImGui_BrdigeRenderer::RenderPropManager },
				{ ICON_FA_CLIPBOARD	" Plugins Manager",	&ImGui_BrdigeRenderer::RenderPluginManager },
				{ ICON_FA_TERMINAL	" Console",			&ImGui_BrdigeRenderer::RenderConsole }
			};

			ImGuiThemesManager ThemeManager;

			enum class RendererType : char
			{
				Unknown,

				DirectX9,
				DirectX10,
				DirectX11,

				SDL_OpenGL,
				GLFW_OpenGL,
			};
			RendererType Type{ RendererType::Unknown };
		};

		static inline ImGui_BrdigeRenderer Bridge;

		static inline bool IsOpen = false;

		/// <summary>
		/// Load fonts from 'pleiades/fonts/*.ttf'
		/// </summary>
		static void LoadFonts();
	};
}