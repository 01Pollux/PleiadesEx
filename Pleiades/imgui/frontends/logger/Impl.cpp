
#include "imgui/backends/States.hpp"
#include "plugins/Manager.hpp"

#include "Logger.hpp"

void renderer::global_state::ImGui_BrdigeRenderer::RenderLogger()
{
	static ImGui_JsonLogger manager;

	if (ImGui::Button(ICON_FA_REDO " Refresh"))
		manager.LoadLogs();

	manager.DrawLoggerDesign();
}
