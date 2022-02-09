
#include "plugins/Manager.hpp"
#include "imgui/backends/States.hpp"


void renderer::global_state::ImGui_BrdigeRenderer::RenderPropManager()
{
	auto& callbacks = PropManager.CallbackProps;
	if (imcxx::window_child cur_props{ "Current Props", { 385.f, 0.f }, true, ImGuiWindowFlags_HorizontalScrollbar })
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
	}
	ImGui::SameLine();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	imcxx::group cur_configs_group;

	if (imcxx::window_child cur_props{ "Current Configs", { 0.f, 0.f }, false, ImGuiWindowFlags_HorizontalScrollbar })
	{
		if (PropManager.Current != callbacks.end())
		{
			CallbackState& state = PropManager.Current->second;
			state.RenderInfo(PropManager);
			state._Changed |= state.Callback();
		}
	}
}


auto renderer::global_state::ImGui_BrdigeRenderer::PropManager_t::GetFirstChanged() -> container_iter
{
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
