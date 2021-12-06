
#include "PluginManager.hpp"

SG_NAMESPACE_BEGIN;

void ImGuiPlInfo::DrawPopupState()
{
	if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight))
	{
		for (auto& info : std::array{
			 std::tuple{
				 ICON_FA_STOP " Unload",
				 this->State >= PluginState::Unloaded,
				 ImVec4{ 1.f, 0.23f, 0.31f, 0.34f },
				 &ImGuiPlInfo::Unload
			 },
			 std::tuple{
				 ICON_FA_PLAY " Load",
				 this->State <= PluginState::Loaded,
				 ImVec4{ 0.21f, 1.f, 0.16f, 0.34f },
				 &ImGuiPlInfo::Load
			 },
			 std::tuple{
				 ICON_FA_REDO " Reload",
				 this->State >= PluginState::Unloaded,
				 ImVec4{ 0.21f, 1.f, 0.16f, 0.34f },
				 &ImGuiPlInfo::Reload
			 },
			 std::tuple{
				 ICON_FA_PAUSE_CIRCLE " Pause/Resume",
				 this->State >= PluginState::Unloaded,
				 ImVec4{ 1.f, 0.58f, 0.f, 0.34f },
				 &ImGuiPlInfo::PauseOrResume
			 }
			 })
		{
			ImVec4& clr = std::get<2>(info);
			ImGui::PushStyleColor(ImGuiCol_Header, clr);
			clr.w += 0.1f; ImGui::PushStyleColor(ImGuiCol_HeaderHovered, clr);
			clr.w += 0.1f; ImGui::PushStyleColor(ImGuiCol_HeaderActive, clr);

			if (ImGui::Selectable(std::get<0>(info), false, std::get<1>(info) ? ImGuiSelectableFlags_Disabled : ImGuiSelectableFlags_None))
				(this->*std::get<3>(info))();

			ImGui::PopStyleColor(3);
		}
		ImGui::EndPopup();
	}
}


void ImGuiPlInfo::Load()
{
	// if the plugin is loaded/paused
	if (this->State <= PluginState::Loaded)
		return;

	this->Plugin = SG::plugin_manager.LoadPlugin(this->PluginName);
	this->State = this->Plugin ? (this->Plugin->IsPluginPaused() ? PluginState::Paused : PluginState::Loaded) : PluginState::Failed;
}

void ImGuiPlInfo::Unload()
{
	if (this->Plugin)
	{
		SG::imgui_iface.RemoveCallback(this->Plugin);
		SG::plugin_manager.RequestShutdown(this->Plugin);

		this->Plugin = nullptr;
		this->State = PluginState::Unloaded;
	}
}

void ImGuiPlInfo::Reload()
{
	if (this->State == PluginState::Loaded)
	{
		SG::imgui_iface.RemoveCallback(this->Plugin);
		SG::plugin_manager.RequestShutdown(this->Plugin);

		if (!(this->Plugin = SG::plugin_manager.LoadPlugin(this->PluginName)))
			this->State = PluginState::Failed;
	}
}

void ImGuiPlInfo::PauseOrResume()
{
	switch (this->State)
	{
	case PluginState::Paused:
	{
		this->Plugin->SetPluginState(false);
		this->State = PluginState::Loaded;
		break;
	}
	case PluginState::Loaded:
	{
		this->Plugin->SetPluginState(true);
		this->State = PluginState::Paused;
		break;
	}
	default: break;
	}
}


void ImGuiPlInfo::DrawPluginsProps()
{
	auto& props = SG::imgui_iface.GetPropManager();
	
	if (ImGui::CollapsingHeader("Plugin Props"))
	{
		for (auto& prop : props.CallbackProps)
		{
			auto& info = prop.second;
			if (info.Plugin != this->Plugin)
				continue;

			ImGui::BulletText("%s: ", prop.first);
			ImGui::PushID(info.Id);
			ImGui::Indent();

			info._Changed |= info.Callback();

			ImGui::Unindent();
			ImGui::PopID();

			ImGui::NewLine();
		}
	}
}


SG_NAMESPACE_END;