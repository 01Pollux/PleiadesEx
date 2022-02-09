
#include "library/Manager.hpp"
#include "logs/Logger.hpp"

#include "PluginManager.hpp"
#include "imgui/backends/States.hpp"


static void DrawPluginStateToImGui(PluginState state);

NLOHMANN_JSON_SERIALIZE_ENUM(
	PluginState, {
	{ PluginState::Unloaded, nullptr },
	{ PluginState::Paused, "paused"},
	{ PluginState::Loaded, "loaded"},
	{ PluginState::Unloaded, "unloaded"},
	{ PluginState::Failed, "failed" }
});


void ImGui_PluginManager::LoadPlugins()
{
	m_PlManSection.Plugins.clear();

	try
	{
		std::string path = px::lib_manager.GoToDirectory(px::PlDirType::Plugins);
		if (path.empty())
		{
			m_PlManSection.reset();
			return;
		}

		bool ignored_this_plugin = false;
		namespace fs = std::filesystem;
		for (auto& dir : fs::directory_iterator(path))
		{
			if (!dir.is_directory())
				continue;

			std::string file = dir.path().stem().string();
			if (!ignored_this_plugin && file.find("ImGui Interface") != std::string::npos)
			{
				ignored_this_plugin = true;
				continue;
			}

			px::IPlugin* pl = px::plugin_manager.FindPlugin(file);

			ImGuiPlInfo info{
				std::move(file),
				pl,
				{ },
				pl ? (pl->IsPluginPaused() ? PluginState::Paused : PluginState::Loaded) : PluginState::Unloaded
			};

			m_PlManSection.Plugins.emplace_back(std::move(info));
		}

		m_PlManSection.Plugins.shrink_to_fit();
		m_PlManSection.reset();
	}
	catch (const std::exception& ex)
	{
		PX_LOG_MESSAGE(
			PX_MESSAGE("Exception reported while loading Packs"),
			PX_LOGARG("Exception", ex.what())
		);
		m_PlManSection.reset();
		return;
	}
}


void ImGui_PluginManager::DrawPluginDesign()
{
	using enum PluginState;

	if (imcxx::window_child cur_props{ "Current Props", { 385.f, 0.f }, true })
	{
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("A").x * 15);

		static bool allow_states[static_cast<size_t>(PluginState::Failed) + 1]{ true, true, true, true };
		if (imcxx::combo_box states{ "##States", "States", ImGuiComboFlags_::ImGuiComboFlags_PopupAlignLeft })
		{
			size_t i = 0;
			for (auto& type : {
				PlStateToString(PluginState::Paused),
				PlStateToString(PluginState::Loaded),
				PlStateToString(PluginState::Unloaded),
				PlStateToString(PluginState::Failed)
			})
				ImGui::Selectable(type, &allow_states[i++], ImGuiSelectableFlags_DontClosePopups);
		}

		ImGui::SameLine(); m_PlManSection.Filter.Draw("", -25.f);
		ImGui::SameLineHelp("Filter (inc, -exc)");

		for (auto iter = m_PlManSection.Plugins.begin(); iter != m_PlManSection.Plugins.end(); iter++)
		{
			if (!allow_states[static_cast<size_t>(iter->State)])
				continue;

			if (!m_PlManSection.Filter.PassFilter(iter->PluginName.c_str()))
				continue;

			bool selected = m_PlManSection.Current == iter;

			if (ImGui::Selectable(iter->PluginName.c_str(), selected))
				m_PlManSection.Current = iter;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

	}

	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	imcxx::group config_group;

	{
		if (imcxx::window_child cur_config{ "Current Configs", { 0, -ImGui::GetFrameHeightWithSpacing() } })
		{
			if (m_PlManSection.Current != m_PlManSection.Plugins.end())
			{
				ImGuiPlInfo& plinfo = *m_PlManSection.Current;
				plinfo.DrawPopupState();

				const float textwidth = ImGui::CalcTextSize("A").x * 14.2f;

				if (imcxx::table info_table{ "##Plugins Info", 2, ImGuiTableFlags_Borders })
				{
					info_table.setup(
						imcxx::table::setup_no_row{}, 
						imcxx::table::setup_info{ nullptr, ImGuiTableColumnFlags_WidthFixed, textwidth },
						imcxx::table::setup_info{ nullptr, ImGuiTableColumnFlags_WidthStretch }
					);

					for (auto& info : std::array{
							std::pair{ "Author",		plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Author : "???" },
							std::pair{ "Name",			plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Name : "???" },
							std::pair{ "Description",	plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Description : "???" }
						 })
					{
						if (info_table.next_column())
							ImGui::TextUnformatted(info.first);
						if (info_table.next_column())
							ImGui::TextUnformatted(info.second);
					}

					if (info_table.next_column())
						ImGui::TextUnformatted("Version");
					{
						const px::version& ver = plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Version : px::version{ };

						ImVec4 clr{
							plinfo.State == Loaded ? ImVec4{ 0.f, 1.f, 0.f, 1.f } : // Loaded and an up to date version
							plinfo.State == Paused ? ImVec4{ 1.f, 0.58f, 0.f, 1.f } : // Paused
							ImVec4{ 1.f, 0.f, 0.f, 1.f } // Failed / Unloaded
						};

						if (info_table.next_column())
							ImGui::Text(plinfo.Plugin ? "%i.%i.%i.%i" : "???", PlVersionFmt(ver));

						DrawPluginStateToImGui(plinfo.State);
					}
				}

				// if the plugin isn't loaded nor paused
				if (plinfo.State <= PluginState::Loaded)
					plinfo.DrawPluginsProps();
			}
		}
	}
}

void DrawPluginStateToImGui(PluginState state)
{
	if (ImGui::TableNextColumn())
		ImGui::TextUnformatted("State");
	if (ImGui::TableNextColumn())
	{
		ImVec4 clr{
			state == PluginState::Loaded ? ImVec4{ 0.f, 1.f, 0.f, 1.f } :
			state == PluginState::Paused ? ImVec4{ 1.f, 0.58f, 0.f, 1.f } :
			ImVec4{ 1.f, 0.f, 0.f, 1.f } // Failed/Unloaded
		};

		ImGui::TextColored(clr, PlStateToString(state));
	}
}
