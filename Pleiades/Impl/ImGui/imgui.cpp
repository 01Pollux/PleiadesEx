
#include "Impl/Interfaces/Logger.hpp"
#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Plugins/PluginManager.hpp"
#include "imgui_iface.hpp"

SG_NAMESPACE_BEGIN;

bool ImGuiInterface::LoadImGui(const nlohmann::json& cfg)
{
	auto windows = cfg.find("windows");
	if (windows == cfg.end())
		return false;

	m_ProcWindowName.set_key(SG::lib_manager.GetHostName().c_str());

	m_ProcWindowName.from_json(*windows);
	if (m_ProcWindowName->empty())
		return false;

	m_Renderer.ThemeManager.ReloadThemes();

	m_ImGuiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_ImGuiContext);

	if (!this->InitializeWindow(cfg))
	{
		this->ShutdownWindow(false);
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	return true;
}

void ImGuiInterface::SaveImGui(nlohmann::json& cfg)
{
	if (auto theme = m_Renderer.ThemeManager.CurrentTheme; theme)
		cfg["theme"] = *theme;
	m_Renderer.SaveTabs(cfg["tabs"]);
}

SG_NAMESPACE_END;