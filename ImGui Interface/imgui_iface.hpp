#pragma once

#include <boost/system.hpp>

#include "Interfaces/InterfacesSys.hpp"
#include "Defines.hpp"
#include "User/IntPtr.hpp"

#include "Interfaces/ImGui.hpp"
#include "Render/render.hpp"


#if BOOST_WINDOWS

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <imgui/backends/imgui_impl_dx9.h>
#include <imgui/backends/imgui_impl_win32.h>

#elif BOOST_LINUX

#pragma error("Add linux support for ImGui!")

#endif


class ImGuiInterface final : public SG::IPluginImpl, public SG::IImGuiLoader
{
public:
	void OnPluginPreLoad(SG::IPluginManager* ifacemgr) override;

	bool OnPluginLoad2(SG::IPluginManager* ifacemgr) override;

	void OnSaveConfig(Json& cfg) override;

	void OnReloadConfig(const Json& cfg) override;

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
	void InitializeWindow(const std::string& default_theme);

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
