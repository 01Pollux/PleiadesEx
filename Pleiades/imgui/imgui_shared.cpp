#include <fstream>

#include "backends/States.hpp"
#include "library/Manager.hpp"
#include "logs/logger.hpp"
#include "imgui_iface.hpp"


ImGuiContext* ImGuiInterface::GetContext() noexcept
{
	return ImGui::GetCurrentContext();
}

px::ImGuiPlCallbackId ImGuiInterface::AddCallback(px::IPlugin* plugin, const char* name, const px::ImGuiPluginCallback& callback)
{
	auto& callbacks = renderer::global_state::Bridge.PropManager.CallbackProps;

	px::ImGuiPlCallbackId id = 0;
	for (const auto& cb : callbacks)
	{
		if (cb.second.Id == id)
			++id;
	}
	callbacks.emplace(name, renderer::global_state::ImGui_BrdigeRenderer::CallbackState{ callback, plugin, id });

	return id;
}

void ImGuiInterface::RemoveCallback(const px::ImGuiPlCallbackId id)
{
	auto& prop = renderer::global_state::Bridge.PropManager;
	auto iter = std::find_if(prop.CallbackProps.begin(), prop.CallbackProps.end(), [id] (const auto& it) { return it.second.Id == id; });

	if (iter != prop.CallbackProps.end())
	{
		prop.CallbackProps.erase(iter);
		if (prop.Current == iter)
			prop.Current = prop.CallbackProps.end();
	}
}

void ImGuiInterface::RemoveCallback(const px::IPlugin* plugin) noexcept
{
	auto& prop = renderer::global_state::Bridge.PropManager;
	bool should_invalidate = false;
	for (auto iter = prop.CallbackProps.begin(); iter != prop.CallbackProps.end(); iter++)
	{
		if (iter->second.Plugin != plugin)
			continue;
		prop.CallbackProps.erase(iter);
		if (prop.Current == iter)
			should_invalidate = true;
	}

	if (should_invalidate)
		prop.Current = prop.CallbackProps.end();
}

bool ImGuiInterface::IsMenuOn()
{
	return m_WindowsIsOn;
}

ImFont* ImGuiInterface::FindFont(const char* font_name)
{
	auto iter = m_LoadedFonts.find(font_name);
	return iter != m_LoadedFonts.end() ? iter->second.Font : nullptr;
}
