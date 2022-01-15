
#include <atomic>

#include <px/interfaces/GameData.hpp>

#include "Manager.hpp"
#include "plugins/Manager.hpp"
#include "library/Manager.hpp"
#include "detours/HooksManager.hpp"
#include "Logs/Logger.hpp"

#include "imgui_impl_dx9.hpp"
#include "../win32/imgui_impl_win32.hpp"

#include "../States.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Hook_Test_Recursive();

namespace renderer
{
	struct d3dx9_state
	{
		static inline std::once_flag Init_Once;

		static inline IDirect3DDevice9* RenderDevice;
		static inline HWND		Window;
		static inline WNDPROC	WndProcedure;

		static inline px::IHookInstance
			* Hook_BeginScene,
			* Hook_Reset;
	};

	LRESULT WINAPI WinProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	px::MHookRes On_DeviceReset(px::PassRet* pRet, bool is_post)
	{
		if (is_post)
		{
			if (SUCCEEDED(pRet->get<HRESULT>()))
				ImGui_ImplDX9_CreateDeviceObjects();
			else IM_ASSERT(!"IDirect3DDevice9::Reset() Failure.");
		}
		else
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
		}
		
		return { };
	}

	px::MHookRes Pre_BeginScene(px::PassArgs* pArgs) 
	{
		std::call_once(
			d3dx9_state::Init_Once,
			[pArgs]()
			{
				d3dx9_state::RenderDevice = pArgs->get<IDirect3DDevice9*>(0);

				ImGui_ImplWin32_Init(d3dx9_state::Window);
				ImGui_ImplDX9_Init(d3dx9_state::RenderDevice);

				d3dx9_state::WndProcedure = std::bit_cast<WNDPROC>(SetWindowLongPtr(d3dx9_state::Window, GWLP_WNDPROC, std::bit_cast<LONG>(&WinProcCallback)));

				d3dx9_state::Hook_Reset->AddCallback(
					false,
					px::HookOrder::ReservedFirst,
					std::bind(&On_DeviceReset, std::placeholders::_1, false)
				);
				d3dx9_state::Hook_Reset->AddCallback(
					true,
					px::HookOrder::ReservedFirst,
					std::bind(&On_DeviceReset, std::placeholders::_1, true)
				);
			}
		);
		if (d3dx9_state::RenderDevice != pArgs->get<IDirect3DDevice9*>(0))
			return { };

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (global_state::IsOpen)
		{
			constexpr ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

			ImGui::SetNextWindowSize({ 1400, 760 }, ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Pleiades", &global_state::IsOpen, main_flags))
				global_state::Bridge.RenderAll();

			ImGui::End();


			if (ImGui::Begin("Hook Rec"))
			{
				Hook_Test_Recursive();
			}

			ImGui::End();
		}

		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		return { };
	}

	LRESULT WINAPI WinProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_KEYUP && wParam == VK_END)
			global_state::IsOpen = !global_state::IsOpen;

		if (!global_state::IsOpen)
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);

		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return global_state::IsOpen ? TRUE : DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	bool InitializeForDx9(const nlohmann::json& data)
	{
		auto window_class_iter = data.find("class");
		auto window_name_iter = data.find("window");

		const std::string* pwindow_class = nullptr;
		if (window_class_iter != data.end() && window_class_iter->is_string())
			pwindow_class = window_class_iter->get_ptr<const std::string*>();
		else pwindow_class = nullptr;

		const std::string* pwindow_name = nullptr;
		if (window_name_iter != data.end() && window_name_iter->is_string())
			pwindow_name = window_name_iter->get_ptr<const std::string*>();
		else pwindow_name = nullptr;


		d3dx9_state::Window = FindWindow(
			pwindow_class ? px::string_cast<px::tstring>(*pwindow_class).c_str() : nullptr,
			pwindow_name ? px::string_cast<px::tstring>(*pwindow_name).c_str() : nullptr
		);
		if (!d3dx9_state::Window)
		{
			PX_LOG_FATAL(
				PX_MESSAGE("Failed to find window by class name"),
				PX_LOGARG("Name", (pwindow_name ? *pwindow_name : "")),
				PX_LOGARG("Class", (pwindow_class ? *pwindow_class : ""))
			);
			return false;
		}

		{
			std::unique_ptr<px::IGameData> pData(px::lib_manager.OpenGameData(nullptr));

			std::vector<std::string> direct_3dx9{ "IDirect3DDevice9" };
			d3dx9_state::Hook_BeginScene = px::detour_manager.LoadHook(direct_3dx9, "BeginScene", nullptr, pData.get(), nullptr, nullptr);
			d3dx9_state::Hook_Reset = px::detour_manager.LoadHook(direct_3dx9, "Reset", nullptr, pData.get(), nullptr, nullptr);

			if (!d3dx9_state::Hook_BeginScene || !d3dx9_state::Hook_Reset)
			{
				PX_LOG_FATAL(
					PX_MESSAGE("Failed to load hook for IDirect3DDevice9::BeginScene/Reset")
				);
				return false;
			}
		}

		d3dx9_state::Hook_BeginScene->AddCallback(
			false,
			px::HookOrder::ReservedFirst,
			std::bind(&Pre_BeginScene, std::placeholders::_2)
		);

		return true;
	}

	void ShutdownForDx9()
	{
		px::detour_manager.ReleaseHook(d3dx9_state::Hook_BeginScene);
		px::detour_manager.ReleaseHook(d3dx9_state::Hook_Reset);

		if (d3dx9_state::WndProcedure)
		{
			SetWindowLongPtr(d3dx9_state::Window, GWLP_WNDPROC, std::bit_cast<LONG>(d3dx9_state::WndProcedure));
			d3dx9_state::WndProcedure = nullptr;

			ImGui_ImplDX9_Shutdown();
			ImGui_ImplWin32_Shutdown();
		}
	}
}
