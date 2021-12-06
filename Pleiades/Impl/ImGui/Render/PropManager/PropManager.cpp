#include "Impl/Plugins/PluginManager.hpp"
#include "../../Render/render.hpp"

SG_NAMESPACE_BEGIN;

void ImGui_BrdigeRenderer::CallbackState::RenderInfo(PropManager_t& prop_manager)
{
	const ImVec2 size{ (ImGui::GetContentRegionAvail().x - 12.f) / 3, 0.f };

	// Only save when we actually change something
	if (ImGui::Button(ICON_FA_SAVE " Save", size) && this->_Changed)
	{
		SG::plugin_manager.UpdatePluginConfig(this->Plugin, PlCfgLoadType::Save);

		for (auto& [key, info] : prop_manager.CallbackProps)
			if (info.Plugin == this->Plugin)
				info._Changed = false;
	}
	ImGui::SameLine(0, 12.f);

	// Load/Reload if the user changed something in config
	if (ImGui::Button(ICON_FA_ARROW_CIRCLE_DOWN " Load", size))
	{
		SG::plugin_manager.UpdatePluginConfig(this->Plugin, PlCfgLoadType::Load);
		
		for (auto& [key, info] : prop_manager.CallbackProps)
			if (info.Plugin == this->Plugin)
				info._Changed = false;
	}

	ImGui::SameLine(0, 12.f);
	if (ImGui::Button(ICON_FA_INFO_CIRCLE " About", size))
		ImGui::OpenPopup(ICON_FA_INFO_CIRCLE " About");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(.5f, .5f));

	if (ImGui::BeginPopupModal(ICON_FA_INFO_CIRCLE " About", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const PluginInfo* pInfo = this->Plugin->GetPluginInfo();
		if (ImGui::BeginTable("Plugin Info", 2, ImGuiTableFlags_Borders))
		{
			for (auto info : std::array{ 
				std::pair{ ICON_FA_QUESTION " Name", pInfo->m_Name },
				std::pair{ ICON_FA_USER " Author", pInfo->m_Author },
				std::pair{ ICON_FA_INFO_CIRCLE " Description", pInfo->m_Description },
				std::pair{ ICON_FA_CALENDAR " Date", pInfo->m_Date }
			 })
			{
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(info.first);

				ImGui::TableNextColumn();
				ImGui::TextUnformatted(info.second);
			}

			ImGui::TableNextColumn();
			ImGui::Text(ICON_FA_INFO_CIRCLE " Version");

			ImGui::TableNextColumn();
			Version ver = pInfo->m_Version;
			ImGui::Text("%i.%i.%i.%i", PlVersionFmt(ver));

			ImGui::EndTable();
		}

		if (ImGui::Button("Close", { -FLT_MIN, 0 }))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

bool ImGui_BrdigeRenderer::CallbackState::DisplayPopupShutown(key_type name, PropManager_t& prop_manager)
{
	std::string str = std::format(ICON_FA_EXCLAMATION_CIRCLE " Save \"{}\"?", name);

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(.5f, .5f));

	bool continue_closing = true;

	if (!ImGui::IsPopupOpen(str.c_str()))
		ImGui::OpenPopup(str.c_str());

	if (ImGui::BeginPopupModal(str.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const float
			avail_size = ImGui::GetContentRegionAvail().x,
			button_size = (avail_size / 2) - 5.f;

		ImGui::TextUnformatted("Current Changes will be lost.\nDo you want to save the current changes?");
		ImGui::Separator();

		//Draw 'save' buttons
		if (ImGui::Button("Save", { button_size, 0 }))
		{
			SG::plugin_manager.UpdatePluginConfig(this->Plugin, PlCfgLoadType::Save);

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

		if (ImGui::Button("Save for all", { button_size, 0 }))
		{
			for (auto& [key, info] : prop_manager.CallbackProps)
			{
				SG::plugin_manager.UpdatePluginConfig(info.Plugin, PlCfgLoadType::Save);
				info._Changed = false;
				info._SafeToClose = true;
			}
		}

		//Draw 'don't save' buttons
		if (ImGui::Button("Don't Save", { button_size, 0 }))
		{
			this->_Changed = false;
			this->_SafeToClose = true;
		}
		ImGui::SameLine(0, 10.f);

		if (ImGui::Button("Don't Save for all", { button_size, 0 }))
		{
			for (auto& [key, info] : prop_manager.CallbackProps)
			{
				info._Changed = false;
				info._SafeToClose = true;
			}
		}


		if (ImGui::Button("Cancel", { avail_size, 0 }))
		{
			continue_closing = false;

			for (auto& [key, info] : prop_manager.CallbackProps)
				info._SafeToClose = false;
		}

		if (_SafeToClose || !continue_closing)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	return continue_closing;
}

void ImGui_BrdigeRenderer::RenderPropManager()
{
	auto& callbacks = PropManager.CallbackProps;
	if (ImGui::BeginChild("Current Props", { 385.f, 0.f }, true, ImGuiWindowFlags_HorizontalScrollbar))
	{
		PropManager.Filter.Draw("", -25.f);
		ImGui::SameLineHelp("Filter (inc, -exc)");

		for (auto iter = callbacks.begin(); iter != callbacks.end(); iter++)
		{
			if (!PropManager.Filter.PassFilter(iter->first))
				continue;

			const bool selected = PropManager.Current == iter;
			if (iter->second._Changed)
				ImGui::Bullet();

			if (ImGui::Selectable(iter->first, selected))
				PropManager.Current = iter;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndChild();
		ImGui::SameLine();
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	ImGui::BeginGroup();

	{
		if (ImGui::BeginChild("Current Configs", { 0, 0.f }, false, ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (PropManager.Current != callbacks.end())
			{
				CallbackState& state = PropManager.Current->second;
				state.RenderInfo(PropManager);
				state._Changed |= state.Callback();
			}
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}


ImGui_BrdigeRenderer::container_iter ImGui_BrdigeRenderer::PropManager_t::GetFirstChanged()
{
	std::vector<container_iter> iters;

	for (auto iter = CallbackProps.begin(); iter != CallbackProps.end(); iter++)
	{
		if (iter->second._Changed)
		{
			if (iter->second._SafeToClose)
				continue;
			return iter;
		}
	}

	return CallbackProps.end();
}

SG_NAMESPACE_END;