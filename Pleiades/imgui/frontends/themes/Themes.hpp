#pragma once

#include <nlohmann/Json.hpp>
#include <px/defines.hpp>

class ImGuiThemesManager
{
public:
	struct Theme
	{
		std::string Name;
		nlohmann::json Info;
	};

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
	void SetColors(const Theme& theme);

	std::vector<Theme> m_LoadedThemes;
};
