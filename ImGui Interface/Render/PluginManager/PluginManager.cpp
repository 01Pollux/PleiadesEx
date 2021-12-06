
#include <filesystem>
#include <fstream>

#include "PluginManager.hpp"
#include "User/Version_Fmt.hpp"


SG_NAMESPACE_BEGIN;

static void DrawPluginStateToImGui(PluginState state);

NLOHMANN_JSON_SERIALIZE_ENUM(
	PluginState, {
	{ PluginState::Unloaded, nullptr },
	{ PluginState::Paused, "paused"},
	{ PluginState::Loaded, "loaded"},
	{ PluginState::Unloaded, "unloaded"},
	{ PluginState::Failed, "failed" }
});


void ImGui_BrdigeRenderer::RenderPluginManager()
{
	static ImGui_PluginManager manager(static_cast<ImGuiInterface*>(SG::ThisPlugin)->GetWindowName());

	ImGui::Text(ICON_FA_INFO_CIRCLE " Host Name: %s", manager.HostName.data());
	ImGui::Text(ICON_FA_INFO_CIRCLE " Host Version: %i.%i.%i.%i", PlVersionFmt(manager.HostVer));

	if (ImGui::Button(ICON_FA_REDO " Refresh"))
		manager.LoadPlugins();

	manager.DrawPluginDesign();
}


void ImGui_PluginManager::LoadPlugins()
{
	m_PlManSection.Plugins.clear();

	char path[MAX_PATH]{ };
	if (!SG::LibManager->GoToDirectory(SG::PlDirType::Plugins, nullptr, path, std::ssize(path)))
	{
		SG_LOG_MESSAGE(
			SG_MESSAGE("Exception reported while loading Packs"),
			SG_LOGARG("Exception", path)
		);
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

		IPlugin* pl = SG::PluginManager->FindPlugin(file);

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


void ImGui_PluginManager::DrawPluginDesign()
{
	using enum PluginState;

	if (ImGui::BeginChild("Current Props", { 385.f, 0.f }, true))
	{
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("A").x * 15);

		static bool allow_states[static_cast<size_t>(PluginState::Failed) + 1]{ true, true, true, true };
		if (ImGui::BeginCombo("##States", "States", ImGuiComboFlags_::ImGuiComboFlags_PopupAlignLeft))
		{
			size_t i = 0;
			for (auto& type : {
				PlStateToString(PluginState::Paused),
				PlStateToString(PluginState::Loaded),
				PlStateToString(PluginState::Unloaded),
				PlStateToString(PluginState::Failed)
			})
				ImGui::Selectable(type, &allow_states[i++], ImGuiSelectableFlags_DontClosePopups);

			ImGui::EndCombo();
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

		ImGui::EndChild();
		ImGui::SameLine();
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	ImGui::BeginGroup();
	{
		if (ImGui::BeginChild("Current Configs", { 0, -ImGui::GetFrameHeightWithSpacing() }))
		{
			if (m_PlManSection.Current != m_PlManSection.Plugins.end())
			{
				ImGuiPlInfo& plinfo = *m_PlManSection.Current;
				plinfo.DrawPopupState();

				const float textwidth = ImGui::CalcTextSize("A").x * 14.2f;

				if (ImGui::BeginTable("##Plugins Info", 2, ImGuiTableFlags_Borders))
				{
					ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, textwidth);
					ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					for (auto& info : std::array{
							std::pair{ "Author",		plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Author : "???" },
							std::pair{ "Name",			plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Name : "???" },
							std::pair{ "Description",	plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Description : "???" }
						 })
					{
						if (ImGui::TableNextColumn())
							ImGui::TextUnformatted(info.first);
						if (ImGui::TableNextColumn())
							ImGui::TextUnformatted(info.second);
					}

					if (ImGui::TableNextColumn())
						ImGui::TextUnformatted("Version");
					{
						const Version& ver = plinfo.Plugin ? plinfo.Plugin->GetPluginInfo()->m_Version : Version{ };

						ImVec4 clr{
							plinfo.State == Loaded ? ImVec4{ 0.f, 1.f, 0.f, 1.f } : // Loaded and an up to date version
							plinfo.State == Paused ? ImVec4{ 1.f, 0.58f, 0.f, 1.f } : // Paused
							ImVec4{ 1.f, 0.f, 0.f, 1.f } // Failed / Unloaded
						};

						if (ImGui::TableNextColumn())
							ImGui::Text(plinfo.Plugin ? "%i.%i.%i.%i" : "???", PlVersionFmt(ver));

						DrawPluginStateToImGui(plinfo.State);
					}

					ImGui::EndTable();
				}

				// if the plugin isn't loaded nor paused
				if (plinfo.State <= PluginState::Loaded)
					plinfo.DrawPluginsProps();
			}
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
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

SG_NAMESPACE_END;
