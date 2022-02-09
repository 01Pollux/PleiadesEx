
#include <filesystem>
#include <fstream>

#include "library/Manager.hpp"
#include "Themes.hpp"

#include <imgui/imgui.h>
#include <px/icons/FontAwesome.hpp>

#include "logs/Logger.hpp"


void ImGuiThemesManager::ReloadThemes()
{
	this->m_LoadedThemes.clear();
	try
	{
		if (std::string path = px::lib_manager.GoToDirectory(px::PlDirType::Main, "themes"); !path.empty())
		{
			namespace fs = std::filesystem;
			for (auto& dir : fs::directory_iterator(path))
			{
				if (dir.path().extension() != ".json")
					continue;

				std::string name = dir.path().stem().string();

				std::ifstream file(dir.path());
				auto cfg{ nlohmann::json::parse(file, nullptr, false, true) };

				if (!cfg.is_discarded())
					this->m_LoadedThemes.emplace_back(dir.path().stem().string(), std::move(cfg));
			}
		}
	}
	catch (const std::exception& ex)
	{
		PX_LOG_MESSAGE(
			PX_MESSAGE("Exception reported while reloading themes"),
			PX_LOGARG("Exception", ex.what())
		);
	}
	
	this->CurrentTheme = nullptr;
}


void ImGuiThemesManager::Render()
{
	for (auto& theme : m_LoadedThemes)
	{
		if (ImGui::MenuItem(theme.Name.c_str()))
			this->SetColors(theme);
	}

	ImGui::Separator();
	if (ImGui::MenuItem(ICON_FA_REDO " Refresh"))
		this->ReloadThemes();
}


void ImGuiThemesManager::LoadTheme(const std::string& name)
{
	if (name.empty())
		return;
	for (auto& theme : m_LoadedThemes)
	{
		if (name == theme.Name)
		{
			this->SetColors(theme);
			break;
		}
	}
}


void ImGuiThemesManager::SetColors(const Theme& theme)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* clrs = style.Colors;
	this->CurrentTheme = &theme.Name;

	if (auto colors = theme.Info.find("colors"); colors != theme.Info.end())
	{
		for (size_t i = 0; i < ImGuiCol_COUNT; i++)
		{
			const char* name = ImGui::GetStyleColorName(i);
			auto iter = colors->find(name);
			if (iter == colors->end() || !iter->is_array())
				continue;

			const auto& arr = *iter;
			if (arr.size() >= 4)
			{
				clrs[i] = {
					static_cast<float>(arr[0]) / 255,
					static_cast<float>(arr[1]) / 255,
					static_cast<float>(arr[2]) / 255,
					static_cast<float>(arr[3]) / 255
				};
			}
		}
	}

	constexpr float* fnullptr = nullptr;
	for (auto& name_settings : {
		//	{ setting's name, { [float*, nullptr]/[float*, float*] }
		std::pair{ "Alpha",				std::pair{ &style.Alpha,				fnullptr } },
		std::pair{ "DisabledAlpha",		std::pair{ &style.DisabledAlpha,		fnullptr } },
		std::pair{ "WindowPadding",		std::pair{ &style.WindowPadding.x,		&style.WindowPadding.y } },
		std::pair{ "WindowRounding",	std::pair{ &style.WindowRounding,		fnullptr } },
		std::pair{ "WindowBorderSize",	std::pair{ &style.WindowBorderSize,		fnullptr } },
		std::pair{ "WindowMinSize",		std::pair{ &style.WindowMinSize.x,		&style.WindowMinSize.y } },
		std::pair{ "WindowTitleAlign",	std::pair{ &style.WindowTitleAlign.x,	&style.WindowTitleAlign.y } },
		std::pair{ "ChildBorderSize",	std::pair{ &style.ChildBorderSize,		fnullptr } },
		std::pair{ "PopupRounding",		std::pair{ &style.PopupRounding,		fnullptr } },
		std::pair{ "PopupBorderSize",	std::pair{ &style.PopupBorderSize,		fnullptr } },
		std::pair{ "FramePadding",		std::pair{ &style.FramePadding.x,		&style.FramePadding.y } },
		std::pair{ "FrameRounding",		std::pair{ &style.FrameRounding,		fnullptr } },
		std::pair{ "FrameBorderSize",	std::pair{ &style.FrameBorderSize,		fnullptr } },
		std::pair{ "ItemSpacing",		std::pair{ &style.ItemSpacing.x,		&style.ItemSpacing.y } },
		std::pair{ "ItemInnerSpacing",	std::pair{ &style.ItemInnerSpacing.x,	&style.ItemInnerSpacing.y } },
		std::pair{ "CellPadding",		std::pair{ &style.CellPadding.x,		&style.CellPadding.y } },
		std::pair{ "TouchExtraPadding",	std::pair{ &style.TouchExtraPadding.x,	&style.TouchExtraPadding.y } },
		std::pair{ "IndentSpacing",		std::pair{ &style.IndentSpacing,		fnullptr } },
		std::pair{ "ColumnsMinSpacing",	std::pair{ &style.ColumnsMinSpacing,	fnullptr } },
		std::pair{ "ScrollbarSize",		std::pair{ &style.ScrollbarSize,		fnullptr } },
		std::pair{ "ScrollbarRounding",	std::pair{ &style.ScrollbarRounding,	fnullptr } },
		std::pair{ "GrabMinSize",		std::pair{ &style.GrabMinSize,			fnullptr } },
		std::pair{ "GrabRounding",		std::pair{ &style.GrabRounding,			fnullptr } },
		std::pair{ "LogSliderDeadzone",	std::pair{ &style.LogSliderDeadzone,	fnullptr } },
		std::pair{ "TabRounding",		std::pair{ &style.TabRounding,			fnullptr } },
		std::pair{ "TabBorderSize",		std::pair{ &style.TabBorderSize,		fnullptr } },
		std::pair{ "MouseCursorScale",	std::pair{ &style.MouseCursorScale,		fnullptr } },
		std::pair{ "CurveTessellationTol",std::pair{ &style.MouseCursorScale,	fnullptr } },

		std::pair{ "TabMinWidthForCloseButton",std::pair{ &style.TabMinWidthForCloseButton,fnullptr } },
		std::pair{ "ButtonTextAlign",	std::pair{ &style.ButtonTextAlign.x,	&style.ButtonTextAlign.y } },
		std::pair{ "SelectableTextAlign",std::pair{ &style.SelectableTextAlign.x,&style.SelectableTextAlign.y } },
		std::pair{ "DisplayWindowPadding",std::pair{ &style.DisplayWindowPadding.x,&style.DisplayWindowPadding.y } },
		std::pair{ "DisplaySafeAreaPadding",std::pair{ &style.DisplaySafeAreaPadding.x,&style.DisplaySafeAreaPadding.y } },
		})
	{
		if (auto section = theme.Info.find(name_settings.first); section != theme.Info.end())
		{
			std::pair info = name_settings.second;
			// setting is ImVec2
			if (info.second)
			{
				if (section->is_array())
				{
					*info.first = section->at(0).get<float>();
					*info.second = section->at(1).get<float>();
				}
			}
			else if (section->is_number_float())
			{
				*info.first = section->get<float>();
			}
		}
	}
}
