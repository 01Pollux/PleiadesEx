
#include <array>

#include "library/Manager.hpp"
#include "imgui_iface.hpp"
#include "logs/Logger.hpp"

#include "backends/States.hpp"

#include "backends/dx9/Manager.hpp"

bool ImGuiInterface::LoadImGui(const nlohmann::json& cfg)
{
	auto windows = cfg.find("windows");
	if (windows == cfg.end())
	{
		PX_LOG_FATAL(
			PX_MESSAGE("Failed to find windows section.")
		);
		return false;
	}

	auto cur_proc = windows->find(px::lib_manager.GetHostName());
	if (cur_proc == windows->end())
	{
		PX_LOG_FATAL(
			PX_MESSAGE("Failed to window's host section.")
		);
		return false;
	}
	
	auto render_name = cur_proc->find("renderer");
	if (render_name == cur_proc->end() || !render_name->is_string())
	{
		PX_LOG_FATAL(
			PX_MESSAGE("Failed to host's render type.")
		);
		return false;
	}
	
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	renderer::global_state::LoadFonts();
	renderer::global_state::Bridge.ThemeManager.ReloadThemes();

	{
		auto iter = cfg.find("theme");
		renderer::global_state::Bridge.ThemeManager.LoadTheme((iter == cfg.end() || !iter->is_string()) ? "" : *iter);
	}
	if (auto iter = cfg.find("tabs"); iter != cfg.end() && iter->is_number_integer())
		renderer::global_state::Bridge.LoadTabs(iter->get<uint32_t>());

	using RendererType = renderer::global_state::ImGui_BrdigeRenderer::RendererType;
	using namespace std::string_view_literals;
	for (auto& [name, type] : std::array{
		std::pair{ "directx9"sv, RendererType::DirectX9 },
		std::pair{ "directx10"sv, RendererType::DirectX10 },
		std::pair{ "directx11"sv, RendererType::DirectX11 },
		std::pair{ "sdl_opengl"sv, RendererType::SDL_OpenGL },
		std::pair{ "glfw_opengl"sv, RendererType::GLFW_OpenGL }
	})
	{
		if (!name.compare(*render_name))
		{
			renderer::global_state::Bridge.Type = type;
		}
	}

	switch (renderer::global_state::Bridge.Type)
	{
	case RendererType::Unknown:
	{
		PX_LOG_FATAL(
			PX_MESSAGE("Invalid renderer type.")
		);
		ImGui::DestroyContext();
		return false;
	}
	case RendererType::DirectX9:
	{
		if (!renderer::InitializeForDx9(*cur_proc))
		{
			renderer::ShutdownForDx9();
			ImGui::DestroyContext();
			return false;
		}
		break;
	}
	}
	
	return true;
}

void ImGuiInterface::UnloadImGui()
{
	using RendererType = renderer::global_state::ImGui_BrdigeRenderer::RendererType;
	switch (renderer::global_state::Bridge.Type)
	{
	case RendererType::DirectX9:
	{
		renderer::ShutdownForDx9();
		ImGui::DestroyContext();
		break;
	}
	}
}

void ImGuiInterface::SaveImGui(nlohmann::json& cfg)
{
	if (auto theme = renderer::global_state::Bridge.ThemeManager.CurrentTheme; theme)
		cfg["theme"] = *theme;
	renderer::global_state::Bridge.SaveTabs(cfg["tabs"]);
}