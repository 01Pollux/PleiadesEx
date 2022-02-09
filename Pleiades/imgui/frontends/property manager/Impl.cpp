
#include "imgui/backends/States.hpp"
#include "plugins/Manager.hpp"


bool renderer::global_state::ImGui_BrdigeRenderer::CallbackState::DisplayPopupShutown(key_type name, PropManager_t& prop_manager)
{
	std::string str = std::format(ICON_FA_EXCLAMATION_CIRCLE " Save \"{}\"?", name);

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(.5f, .5f));

	bool continue_closing = true;

	if (!ImGui::IsPopupOpen(str.c_str()))
		ImGui::OpenPopup(str.c_str());

	if (imcxx::popup save_popup{ imcxx::popup::modal{}, str, nullptr, ImGuiWindowFlags_AlwaysAutoResize })
	{
		const float 
			avail_size = ImGui::GetContentRegionAvail().x,
			button_size = (avail_size / 2) - 5.f;

		ImGui::TextUnformatted("Current Changes will be lost.\nDo you want to save the current changes?");
		ImGui::Separator();

		//Draw 'save' buttons
		if (ImGui::Button("Save", { button_size, 0 }))
		{
			px::plugin_manager.UpdatePluginConfig(this->Plugin, px::PlCfgLoadType::Save);

			for (auto& [key, info] : prop_manager.CallbackProps)
			{
				if (info.Plugin == this->Plugin)
				{
					info._Changed = false;
					info._SafeToClose = true;
				}
			}
		}
		ImGui::SameLine(0, 10.f);

		if (ImGui::Button("Save for all", { button_size, 0.f }))
		{
			for (auto& [key, info] : prop_manager.CallbackProps)
			{
				px::plugin_manager.UpdatePluginConfig(info.Plugin, px::PlCfgLoadType::Save);
				info._Changed = false;
				info._SafeToClose = true;
			}
		}

		//Draw 'don't save' buttons
		if (ImGui::Button("Don't Save", { button_size, 0.f }))
		{
			this->_Changed = false;
			this->_SafeToClose = true;
		}
		ImGui::SameLine(0, 10.f);

		if (ImGui::Button("Don't Save for all", { button_size, 0.f }))
		{
			for (auto& [key, info] : prop_manager.CallbackProps)
			{
				info._Changed = false;
				info._SafeToClose = true;
			}
		}


		if (ImGui::Button("Cancel", { avail_size, 0.f }))
		{
			continue_closing = false;

			for (auto& [key, info] : prop_manager.CallbackProps)
				info._SafeToClose = false;
		}

		if (_SafeToClose || !continue_closing)
			save_popup.close();
	}

	return continue_closing;
}


void renderer::global_state::ImGui_BrdigeRenderer::CallbackState::RenderInfo(PropManager_t& prop_manager)
{
	const ImVec2 size{ (ImGui::GetContentRegionAvail().x - 12.f) / 3, 0.f };

	// Only save when we actually change something
	if (ImGui::Button(ICON_FA_SAVE " Save", size) && this->_Changed)
	{
		px::plugin_manager.UpdatePluginConfig(this->Plugin, px::PlCfgLoadType::Save);

		for (auto& [key, info] : prop_manager.CallbackProps)
			if (info.Plugin == this->Plugin)
				info._Changed = false;
	}
	ImGui::SameLine(0, 12.f);

	// Load/Reload if the user changed something in config
	if (ImGui::Button(ICON_FA_ARROW_CIRCLE_DOWN " Load", size))
	{
		px::plugin_manager.UpdatePluginConfig(this->Plugin, px::PlCfgLoadType::Load);

		for (auto& [key, info] : prop_manager.CallbackProps)
			if (info.Plugin == this->Plugin)
				info._Changed = false;
	}

	ImGui::SameLine(0, 12.f);
	if (ImGui::Button(ICON_FA_INFO_CIRCLE " About", size))
		ImGui::OpenPopup(ICON_FA_INFO_CIRCLE " About");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { .5f, .5f });

	if (imcxx::popup about_popup{ imcxx::popup::modal{}, ICON_FA_INFO_CIRCLE " About", nullptr, ImGuiWindowFlags_AlwaysAutoResize })
	{
		const px::PluginInfo* pInfo = this->Plugin->GetPluginInfo();
		if (imcxx::table pl_info_table{ "Plugin Info", 2, ImGuiTableFlags_Borders })
		{
			for (auto info : std::array{
				std::pair{ ICON_FA_QUESTION " Name", pInfo->m_Name },
				std::pair{ ICON_FA_USER " Author", pInfo->m_Author },
				std::pair{ ICON_FA_INFO_CIRCLE " Description", pInfo->m_Description },
				std::pair{ ICON_FA_CALENDAR " Date", pInfo->m_Date }
				})
			{
				if (pl_info_table.next_column())
					ImGui::TextUnformatted(info.first);

				if (pl_info_table.next_column())
					ImGui::TextUnformatted(info.second);
			}

			if (pl_info_table.next_column())
				ImGui::Text(ICON_FA_INFO_CIRCLE " Version");

			if (pl_info_table.next_column())
			{
				px::version ver = pInfo->m_Version;
				ImGui::Text("%i.%i.%i.%i", PlVersionFmt(ver));
			}
		}

		if (ImGui::Button("Close", { -FLT_MIN, 0 }))
			about_popup.close();
	}
}
