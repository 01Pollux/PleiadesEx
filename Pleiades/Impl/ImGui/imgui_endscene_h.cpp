
#include <shadowgarden/interfaces/GameData.hpp>

#include "Impl/Library/LibrarySys.hpp"
#include "Impl/DetoursMan/HooksManager.hpp"
#include "Impl/Plugins/PluginManager.hpp"
#include "Impl/Interfaces/Logger.hpp"
#include "imgui_iface.hpp"

#if BOOST_WINDOWS

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
SG_NAMESPACE_BEGIN;

ImGuiInterface imgui_iface;

bool ImGuiInterface::InitializeWindow(const nlohmann::json& config)
{
	m_CurWindow = FindWindowA(m_ProcWindowName->c_str(), nullptr);
	if (!m_CurWindow)
	{
		SG_LOG_FATAL(
			SG_MESSAGE("Failed to find window by class name"),
			SG_LOGARG("Classname", m_ProcWindowName.get())
		);
		return false;
	}

	std::unique_ptr<SG::IGameData> pData(SG::lib_manager.OpenGameData(nullptr));

	m_D3Dx9EndScene = SG::detour_manager.LoadHook({ "IDirect3DDevice9" }, "EndScene", nullptr, pData.get(), nullptr, nullptr);
	m_D3Dx9Reset = SG::detour_manager.LoadHook({ "IDirect3DDevice9" }, "Reset", nullptr, pData.get(), nullptr, nullptr);
	
	if (!m_D3Dx9EndScene || !m_D3Dx9Reset)
	{
		SG_LOG_FATAL(
			SG_MESSAGE("Failed to load hook for IDirect3DDevice9::Reset/EndScene")
		);
		return false;
	}

	m_OldProc = std::bit_cast<WNDPROC>(SetWindowLongPtrA(m_CurWindow, GWLP_WNDPROC, std::bit_cast<LONG>(&WinProcCallback)));
	ImGui_ImplWin32_Init(m_CurWindow);

	LoadFonts();
	
	namespace ph = std::placeholders;
	m_D3Dx9EndScene->AddCallback(
		false,
		SG::HookOrder::ReservedFirst,
		std::bind(&ImGuiInterface::Pre_DeviceEndScene, this, ph::_1, ph::_2)
	);

	for (bool r : { true, false })
	{
		namespace ph = std::placeholders;

		m_D3Dx9Reset->AddCallback(
			r,
			r ? SG::HookOrder::ReservedLast : SG::HookOrder::ReservedFirst,
			std::bind(&ImGuiInterface::OnDeviceReset, this, ph::_1, ph::_2, r)
		);
	}

	{
		auto iter = config.find("theme");
		this->m_Renderer.ThemeManager.LoadTheme((iter == config.end() || !iter->is_string()) ? "" : *iter);
	}
	if (auto iter = config.find("tabs"); iter != config.end() && iter->is_number_integer())
		this->m_Renderer.LoadTabs(iter->get<uint32_t>());

	return true;
}

void ImGuiInterface::ShutdownWindow(bool unload_all)
{
	if (m_D3Dx9EndScene)
		SG::detour_manager.ReleaseHook(m_D3Dx9EndScene);
	if (m_D3Dx9Reset)
		SG::detour_manager.ReleaseHook(m_D3Dx9Reset);

	if (m_OldProc)
	{
		SetWindowLongPtr(m_CurWindow, GWLP_WNDPROC, std::bit_cast<LONG>(m_OldProc));
		m_OldProc = nullptr;

		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	if (m_ImGuiContext)
		ImGui::DestroyContext(m_ImGuiContext);

	if (unload_all)
		SG::plugin_manager.Shutdown();
}


SG::MHookRes ImGuiInterface::Pre_DeviceEndScene(SG::PassRet* pRet, SG::PassArgs* pArgs)
{
	static std::atomic_bool init = false;
	if (!init)
	{
		init = true;
		ImGui_ImplDX9_Init(pArgs->get<IDirect3DDevice9*>(0));
	}

	SG::MHookRes res;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (m_WindowsIsOn)
	{
		constexpr ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

		ImGui::SetNextWindowSize({ 1400, 760 }, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Pleiades", &m_WindowsIsOn, main_flags) && !m_Renderer.RenderAll())
		{
			// We are shutting down, immediatly exit and restore the hook while not calling the function nor post hooks
			res = SG::BitMask_Or(SG::HookRes::BreakLoop, SG::HookRes::DontCall, SG::HookRes::SkipPost, SG::HookRes::ChangedReturn);
			pRet->set(NULL /*D3D_OK*/);
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// Fix for random crash (race condition) when unloading.
	// while unloading, the original function might returns too late to the old (deleted) address.
	if (res.test(SG::HookRes::BreakLoop))
		ShutdownWindow(true);

	return res;
}


SG::MHookRes ImGuiInterface::OnDeviceReset(SG::PassRet* pRet, SG::PassArgs * pArgs, bool is_post)
{
	if (!is_post)
		ImGui_ImplDX9_InvalidateDeviceObjects();
	else
	{
		if (SUCCEEDED(pRet->get<HRESULT>()))
			ImGui_ImplDX9_CreateDeviceObjects();
	}
	return SG::BitMask_And(SG::HookRes::Ignored);
}


LRESULT WINAPI ImGuiInterface::WinProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYUP && wParam == VK_END)
		SG::imgui_iface.m_WindowsIsOn = !SG::imgui_iface.m_WindowsIsOn;

	if (!SG::imgui_iface.m_WindowsIsOn)
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	return SG::imgui_iface.m_WindowsIsOn ? TRUE : CallWindowProc(SG::imgui_iface.m_OldProc, hWnd, uMsg, wParam, lParam);
}

SG_NAMESPACE_END;

#endif
