
#include "imgui_iface.hpp"

void ImGuiInterface::OnPluginPreLoad(SG::IPluginManager* ifacemgr)
{
	if (!ifacemgr->ExposeInterface(SG::Interface_ImGuiLoader, this, this))
	{
		SG_LOG_ERROR(
			SG_MESSAGE("Tried to load a duplicate Interface"),
			SG_LOGARG("Name", SG::Interface_ImGuiLoader)
		);
	}
	else
	{
		m_ImGuiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_ImGuiContext);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}
}

bool ImGuiInterface::OnPluginLoad2(SG::IPluginManager* ifacemgr)
{
	m_ProcWindowName.set_key(SG::LibManager->GetHostName().c_str());

	ifacemgr->UpdatePluginConfig(this, SG::PlCfgLoadType::Load);

	if (m_ProcWindowName->empty())
	{
		this->ShutdownWindow(false);
		SG_LOG_FATAL(
			SG_MESSAGE("Failed to load hook for IDirect3DDevice9::Reset/EndScene")
		);
		return false;
	}

	return true;
}

void ImGuiInterface::OnSaveConfig(Json& cfg)
{
	if (auto theme = m_Renderer.ThemeManager.CurrentTheme; theme)
		cfg["theme"] = *theme;
}

void ImGuiInterface::OnReloadConfig(const Json& cfg)
{
	if (!m_ProcWindowName->empty())
		return;

	m_ProcWindowName.from_json(cfg);
	if (m_ProcWindowName->empty())
		return;

	m_Renderer.ThemeManager.ReloadThemes();
	auto iter = cfg.find("theme");
	this->InitializeWindow((iter == cfg.end() || !iter->is_string()) ? "" : *iter);
}

SG_DLL_EXPORT(ImGuiInterface);
