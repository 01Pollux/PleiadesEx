#pragma once

#include <nlohmann/Json.hpp>
#include <shadowgarden/defines.hpp>

SG_NAMESPACE_BEGIN;

struct ImGuiTheme
{
	std::string Name;
	nlohmann::json Info;
};

class ImGuiThemesManager
{
public:
	/// <summary>
	/// Load/Reload themes
	/// </summary>
	void ReloadThemes();

	/// <summary>
	/// Display menu items containing themes
	/// </summary>
	void Render();

	/// <summary>
	/// Lookup and load theme by name
	/// </summary>
	void LoadTheme(const std::string& name);

	const std::string* CurrentTheme{ };

private:
	/// <summary>
	/// convert theme from json to ImGuiStyle
	/// </summary>
	void SetColors(const ImGuiTheme& theme);

	std::vector<ImGuiTheme> m_LoadedThemes;
};


SG_NAMESPACE_END;