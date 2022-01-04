#pragma once

#include <boost/system.hpp>

#include <px/intptr.hpp>
#include <px/interfaces/InterfacesSys.hpp>
#include <px/interfaces/HooksManager.hpp>
#include <px/interfaces/ImGui.hpp>

#include "Render/render.hpp"

#if BOOST_WINDOWS

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <imgui/imgui_impl_dx9.h>
#include <imgui/imgui_impl_win32.h>

#elif BOOST_LINUX

#pragma error("Add linux support for ImGui!")

#endif

PX_NAMESPACE_BEGIN();

class ImGuiInterface final : public px::IImGuiLoader
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

	px::ImGuiPlCallbackId AddCallback(px::IPlugin* plugin, const char* name, const px::ImGuiPluginCallback& callback) override;
	
	void RemoveCallback(const px::ImGuiPlCallbackId id) override;

	void RemoveCallback(const px::IPlugin* plugin) noexcept  override;

	bool IsMenuOn() override;

	ImFont* FindFont(const char* font_name) override;

public:
	/// <summary>
	/// More convenient way to get plugin's props for rendering purpose
	/// </summary>
	auto& GetPropManager() noexcept { return m_Renderer.PropManager; }

	const std::string_view GetWindowName() const noexcept { return m_ProcWindowName; }

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
	px::MHookRes Pre_DeviceEndScene(px::PassRet* pRet, px::PassArgs* pArgs);

	px::MHookRes OnDeviceReset(px::PassRet* pRet, px::PassArgs* pArgs, bool is_post);
#endif

private:
#if BOOST_WINDOWS
	HWND	m_CurWindow;
	WNDPROC m_OldProc{ };
	px::IHookInstance* m_D3Dx9EndScene,* m_D3Dx9Reset;
#endif

	ImGuiContext*			 m_ImGuiContext{ };
	px::ImGui_BrdigeRenderer m_Renderer;
	
	std::string				 m_ProcWindowName;
	
	struct FontAndRanges
	{
		ImFont* Font;
		std::unique_ptr<ImWchar[]> Ranges;
	};
	std::unordered_map<std::string, FontAndRanges> m_LoadedFonts;
	bool					m_WindowsIsOn{ };
};

extern ImGuiInterface imgui_iface;

PX_NAMESPACE_END();