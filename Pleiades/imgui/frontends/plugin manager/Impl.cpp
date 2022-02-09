
#include "imgui/backends/States.hpp"
#include "library/Manager.hpp"
#include "PluginManager.hpp"


void renderer::global_state::ImGui_BrdigeRenderer::RenderPluginManager()
{
	static ImGui_PluginManager manager(px::lib_manager.GetHostName());

	ImGui::Text(ICON_FA_INFO_CIRCLE " Host Name: %s", manager.HostName.data());
	ImGui::Text(ICON_FA_INFO_CIRCLE " Host Version: %i.%i.%i.%i", PlVersionFmt(manager.HostVer));

	if (ImGui::Button(ICON_FA_REDO " Refresh"))
		manager.LoadPlugins();

	manager.DrawPluginDesign();
}
