#pragma once

#include <px/interfaces/HooksManager.hpp>
#include <px/interfaces/ImGui.hpp>
#include <nlohmann/json_fwd.hpp>

class ImGuiInterface : public px::IImGuiLoader
{
public:
	[[nodiscard]] bool LoadImGui(const nlohmann::json& cfg);
	
	void UnloadImGui();

	void SaveImGui(nlohmann::json& info);

public:
	// Inherited via IImGuiLoader
	ImGuiContext* GetContext() noexcept override;

	px::ImGuiPlCallbackId AddCallback(px::IPlugin* plugin, const char* name, const px::ImGuiPluginCallback& callback) override;
	
	void RemoveCallback(const px::ImGuiPlCallbackId id) override;

	void RemoveCallback(const px::IPlugin* plugin) noexcept  override;

	bool IsMenuOn() override;

	ImFont* FindFont(const char* font_name) override;

public:
	struct FontAndRanges
	{
		ImFont* Font;
		std::unique_ptr<ImWchar[]> Ranges;
	};
	std::unordered_map<std::string, FontAndRanges>
			m_LoadedFonts;
	bool	m_WindowsIsOn{ };
};

PX_NAMESPACE_BEGIN();
inline ImGuiInterface imgui_iface;
PX_NAMESPACE_END();
