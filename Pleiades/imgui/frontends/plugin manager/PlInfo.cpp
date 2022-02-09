
#include "imgui/backends/States.hpp"
#include "PluginManager.hpp"

void ImGuiPlInfo::DrawPopupState()
{
	if (imcxx::popup plugin_popup{ imcxx::popup::context_window{}, nullptr, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight })
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
			imcxx::shared_color color_override(ImGuiCol_Header, clr);
			clr.w += 0.1f; color_override.push(ImGuiCol_HeaderHovered, clr);
			clr.w += 0.1f; color_override.push(ImGuiCol_HeaderActive, clr);

			if (ImGui::Selectable(std::get<0>(info), false, std::get<1>(info) ? ImGuiSelectableFlags_Disabled : ImGuiSelectableFlags_None))
				(this->*std::get<3>(info))();
		}
	}
}


void ImGuiPlInfo::Load()
{
	// if the plugin is loaded/paused
	if (this->State <= PluginState::Loaded)
		return;

	this->Plugin = px::plugin_manager.LoadPlugin(this->PluginName);
	this->State = this->Plugin ? (this->Plugin->IsPluginPaused() ? PluginState::Paused : PluginState::Loaded) : PluginState::Failed;
}

void ImGuiPlInfo::Unload()
{
	if (this->Plugin)
	{
		px::imgui_iface.RemoveCallback(this->Plugin);
		px::plugin_manager.RequestShutdown(this->Plugin);

		this->Plugin = nullptr;
		this->State = PluginState::Unloaded;
	}
}

void ImGuiPlInfo::Reload()
{
	if (this->State == PluginState::Loaded)
	{
		px::imgui_iface.RemoveCallback(this->Plugin);
		px::plugin_manager.RequestShutdown(this->Plugin);

		if (!(this->Plugin = px::plugin_manager.LoadPlugin(this->PluginName)))
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
	if (imcxx::collapsing_header plugin_props{ "Plugin Props" })
	{
		for (auto& prop : renderer::global_state::Bridge.PropManager.CallbackProps)
		{
			auto& info = prop.second;
			if (info.Plugin != this->Plugin)
				continue;

			ImGui::BulletText("%s: ", prop.first);
			{
				imcxx::shared_item_id info_id(info.Id);
				imcxx::indent indent;
				info._Changed |= info.Callback();
			}
			ImGui::NewLine();
		}
	}
}
