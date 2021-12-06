#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Plugins/PluginManager.hpp"
#include "render.hpp"

SG_NAMESPACE_BEGIN;

bool ImGui_BrdigeRenderer::RenderAll()
{
	static bool 
		is_shutting_down = false,
		show_about = false,
		show_style_editor;

	if (show_about)
	{
		if (!ImGui::IsPopupOpen("##About"))
			ImGui::OpenPopup("##About");

		if (ImGui::BeginPopupModal("##About", &show_about))
		{
			ImGui_BrdigeRenderer::RenderAbout();
			ImGui::EndPopup();
		}
	}
	else if (is_shutting_down)
	{
		auto first_pending = PropManager.GetFirstChanged();

		if (first_pending != PropManager.CallbackProps.end())
		{
			auto& info = first_pending->second;
			if (!info.DisplayPopupShutown(first_pending->first, PropManager))
				is_shutting_down = false;
		}
		else if (is_shutting_down)
		{
			is_shutting_down = false;
			return false;
		}
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("More..."))
		{
			if (ImGui::MenuItem(ICON_FA_PAINT_BRUSH " Style editor"))
				show_style_editor = true;

			if (ImGui::BeginMenu(ICON_FA_PALETTE " Themes"))
			{
				ThemeManager.Render();
				ImGui::EndMenu();
			}
			
			if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE " About"))
				show_about = true;

			if (ImGui::MenuItem(ICON_FA_POWER_OFF " Quit"))
				is_shutting_down = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tabs"))
		{
			for (auto& tabinfo : MainTabsInfo)
				ImGui::Selectable(tabinfo.Name, &tabinfo.isOpen, ImGuiSelectableFlags_DontClosePopups);
			
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	const ImGuiID dock_id = ImGui::GetID("Main Tab");
	ImGui::DockSpace(dock_id);

	constexpr ImGuiWindowFlags tab_flags = ImGuiWindowFlags_NoCollapse;
	for (auto fn_tab : {
		std::pair{ &ImGui_BrdigeRenderer::RenderLogger,			MainTabsInfo_t::Type::Logger },
		std::pair{ &ImGui_BrdigeRenderer::RenderProfiler,		MainTabsInfo_t::Type::Profiler },
		std::pair{ &ImGui_BrdigeRenderer::RenderPropManager,	MainTabsInfo_t::Type::PropManager },
		std::pair{ &ImGui_BrdigeRenderer::RenderPluginManager,	MainTabsInfo_t::Type::PluginsManager }
	})
	{
		auto& tab = GetTab(fn_tab.second);
		if (!tab.isOpen)
			continue;

		ImGui::SetNextWindowDockID(dock_id, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(tab.Name, &tab.isOpen, tab_flags))
		{
			ImGui::PushID(&tab);
			(this->*fn_tab.first)();
			ImGui::PopID();
		}

		ImGui::End();
	}

	if (show_style_editor)
	{
		if (ImGui::Begin("Style editor", &show_style_editor, tab_flags))
			ImGui::ShowStyleEditor();

		ImGui::End();
	}

	return true;
}


void ImGui_BrdigeRenderer::RenderAbout()
{
	{
		ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
		ImGui::Separator();
		ImGui::Text("By Omar Cornut and all Dear ImGui contributors.");
		ImGui::Text("Dear ImGui is licensed under the MIT License, see LICENSE for more information.");
	}
	ImGui::Separator();
	ImGui::NewLine();
	{
		SG::Version ver = SG::plugin_manager.GetHostVersion();
		ImGui::Text("Pleiades Interface: %i.%i.%i.%i", ver.major(), ver.minor(), ver.build(), ver.revision());
		ImGui::Separator();
		ImGui::Text("By 01Pollux/WhiteFalcon.");
		ImGui::Text("licensed under the GNU General Public License v3.0, see LICENSE for more information.");
	}
	ImGui::Separator();
	ImGui::NewLine();
	{
		ImGui::TextUnformatted("ImPlot: 0.12 WIP");
		ImGui::Separator();
		ImGui::Text("By epezent.");
		ImGui::Text("ImPlot is licensed under the MIT License, see LICENSE for more information.");
	}
}

SG_NAMESPACE_END;