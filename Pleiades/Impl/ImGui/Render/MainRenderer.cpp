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

		if (ImGui::BeginPopupModal("##About", &show_about, ImGuiWindowFlags_NoResize))
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
			for (auto& tab : MainTabsInfo)
			{
				if (ImGui::Selectable(tab.Name, &tab.IsOpen, ImGuiSelectableFlags_DontClosePopups))
					tab.IsFocused = true;
			}
			
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	const ImGuiID dock_id = ImGui::GetID("Main Tab");
	ImGui::DockSpace(dock_id);

	constexpr ImGuiWindowFlags tab_flags = ImGuiWindowFlags_NoCollapse;

	for (auto& tab : MainTabsInfo)
	{
		if (!tab.IsOpen)
			continue;

		tab.IsFocused = false;
		if (ImGui::Begin(tab.Name, &tab.IsOpen, tab_flags | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (tab.IsFocused = tab.IsOpen)
			{
				ImGui::PushID(&tab);
				(this->*tab.Callback)();
				ImGui::PopID();
			}
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

// [... Y X Y X ]
// X: Are tabs open flags
// Y: Are tabs focused flags
void ImGui_BrdigeRenderer::LoadTabs(uint32_t serialized_tabs)
{
	for (ptrdiff_t i = 0; i < std::ssize(this->MainTabsInfo); ++i)
	{
		this->MainTabsInfo[i].IsOpen = (serialized_tabs & (1 << (i * 2))) != 0;
		this->MainTabsInfo[i].IsFocused = (serialized_tabs & (1 << (i * 2 + 1))) != 0;
	}
}

void ImGui_BrdigeRenderer::SaveTabs(nlohmann::json& out_config)
{
	uint32_t serialized_tabs{ };
	for (ptrdiff_t i = 0; i < std::ssize(this->MainTabsInfo); ++i)
	{
		if (this->MainTabsInfo[i].IsOpen)
			serialized_tabs |= (1 << (i * 2));
		if (this->MainTabsInfo[i].IsFocused)
			serialized_tabs |= (1 << (i * 2 + 1));
	}
	out_config = serialized_tabs;
}

SG_NAMESPACE_END;