#pragma once

#include <boost/system.hpp>

#include <shadowgarden/users/IntPtr.hpp>
#include <shadowgarden/interfaces/InterfacesSys.hpp>
#include <shadowgarden/interfaces/HooksManager.hpp>
#include <shadowgarden/interfaces/ImGui.hpp>

#include "Render/render.hpp"

#if BOOST_WINDOWS

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <imgui/imgui_impl_dx9.h>
#include <imgui/imgui_impl_win32.h>

#elif BOOST_LINUX

#pragma error("Add linux support for ImGui!")

#endif

SG_NAMESPACE_BEGIN;

class ImGuiInterface final : public SG::IImGuiLoader
{
public:
	bool LoadImGui(const nlohmann::json& cfg);

	void SaveImGui(nlohmann::json& info);

#if BOOST_WINDOWS
	static LRESULT WINAPI WinProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

public:
	// Inherited via IImGuiLoader
	ImGuiContext* GetContext() noexcept override;

	SG::ImGuiPlCallbackId AddCallback(SG::IPlugin* plugin, const char* name, const SG::ImGuiPluginCallback& callback) override;
	
	void RemoveCallback(const SG::ImGuiPlCallbackId id) override;

	void RemoveCallback(const SG::IPlugin* plugin) noexcept  override;

	bool IsMenuOn() override;

	ImFont* FindFont(const char* font_name) override;

public:
	/// <summary>
	/// More convenient way to get plugin's props for rendering purpose
	/// </summary>
	auto& GetPropManager() noexcept { return m_Renderer.PropManager; }

	const std::string_view GetWindowName() const noexcept { return m_ProcWindowName.key(); }

private:
	/// <summary>
	/// Initialize Window and ImGui (Dx9 hooks for windows)
	/// 
	/// After the function return, if 'm_ProcWindowName' is empty, then the initalization failed
	/// </summary>
	bool InitializeWindow(const nlohmann::json& config);

	/// <summary>
	/// Uninitialize Window and ImGui (Dx9 hooks for windows)
	/// </summary>
	void ShutdownWindow(bool unload_all);

	/// <summary>
	/// Load fonts from 'Pleiades/Fonts/*.ttf'
	/// </summary>
	void LoadFonts();

#if BOOST_WINDOWS
	SG::MHookRes Pre_DeviceEndScene(SG::PassRet* pRet, SG::PassArgs* pArgs);

	SG::MHookRes OnDeviceReset(SG::PassRet* pRet, SG::PassArgs* pArgs, bool is_post);
#endif

private:
#if BOOST_WINDOWS
	HWND	m_CurWindow;
	WNDPROC m_OldProc{ };
	SG::IHookInstance* m_D3Dx9EndScene,* m_D3Dx9Reset;
#endif

	ImGuiContext*			 m_ImGuiContext{ };
	SG::ImGui_BrdigeRenderer m_Renderer;
	
	SG::Config<std::string> m_ProcWindowName;
	
	struct FontAndRanges
	{
		ImFont* Font;
		std::unique_ptr<ImWchar[]> Ranges;
	};
	std::unordered_map<std::string, FontAndRanges> m_LoadedFonts;
	bool					m_WindowsIsOn{ };
};

extern ImGuiInterface imgui_iface;

SG_NAMESPACE_END;