
#include <fstream>

#include "library/Manager.hpp"
#include "plugins/Manager.hpp"
#include "Logs/Logger.hpp"

#include <imgui/imcxx/all_in_one.hpp>
#include "States.hpp"

void renderer::global_state::ImGui_BrdigeRenderer::RenderAll()
{
	static bool 
		is_shutting_down = false,
		show_about = false,
		show_style_editor = false;

	if (show_about)
	{
		if (!ImGui::IsPopupOpen("##About"))
			ImGui::OpenPopup("##About");

		imcxx::popup(imcxx::popup::modal{}, "##About", &show_about, ImGuiWindowFlags_NoResize).active_invoke(&ImGui_BrdigeRenderer::RenderAbout, this);
	}
	else if (is_shutting_down)
	{
		auto first_pending = PropManager.GetFirstChanged();

		if (first_pending != PropManager.CallbackProps.end())
		{
			auto& info = first_pending->second;
			if (!info.DisplayPopupShutown(first_pending->first, PropManager))
				is_shutting_down = false;
		}
		else if (is_shutting_down)
		{
			is_shutting_down = false;
			px::plugin_manager.Shutdown();
			return;
		}
	}

	if (imcxx::menubar menu_bar{})
	{
		if (auto more_menu = menu_bar.add_item("More..."))
		{
			if (more_menu.add_entry(ICON_FA_PAINT_BRUSH " Style editor"))
				show_style_editor = true;

			if (auto themes = menu_bar.add_item(ICON_FA_PALETTE " Themes"))
				ThemeManager.Render();
			
			if (more_menu.add_entry(ICON_FA_INFO_CIRCLE " About"))
				show_about = true;

			if (more_menu.add_entry(ICON_FA_POWER_OFF " Quit"))
				is_shutting_down = true;
		}

		if (auto tabs_menu = menu_bar.add_item("Tabs"))
		{
			for (auto& tab : MainTabsInfo)
			{
				if (ImGui::Selectable(tab.Name, &tab.IsOpen, ImGuiSelectableFlags_DontClosePopups))
					tab.IsFocused = true;
			}
		}
	}

	const ImGuiID dock_id = ImGui::GetID("Main Tab");
	ImGui::DockSpace(dock_id);

	constexpr ImGuiWindowFlags tab_flags = ImGuiWindowFlags_NoCollapse;

	for (auto& tab : MainTabsInfo)
	{
		if (!tab.IsOpen)
			continue;

		tab.IsFocused = false;
		if (imcxx::window cur_tab{ tab.Name, &tab.IsOpen, tab_flags | ImGuiWindowFlags_NoFocusOnAppearing })
		{
			if (tab.IsFocused = tab.IsOpen)
			{
				imcxx::shared_item_id tab_id(&tab);
				(this->*tab.Callback)();
			}
		}
	}

	if (show_style_editor)
		imcxx::window("Style editor", &show_style_editor, tab_flags).active_invoke(ImGui::ShowStyleEditor, nullptr);
}


void renderer::global_state::ImGui_BrdigeRenderer::RenderAbout()
{
	{
		ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
		ImGui::Separator();
		ImGui::Text("By Omar Cornut and all Dear ImGui contributors.");
		ImGui::Text("Dear ImGui is licensed under the MIT License, see LICENSE for more information.");
	}
	ImGui::Separator();
	ImGui::NewLine();
	{
		px::version ver = px::plugin_manager.GetHostVersion();
		ImGui::Text("Pleiades Interface: %i.%i.%i.%i", ver.major(), ver.minor(), ver.build(), ver.revision());
		ImGui::Separator();
		ImGui::Text("By 01Pollux/WhiteFalcon.");
		ImGui::Text("licensed under the GNU General Public License v3.0, see LICENSE for more information.");
	}
	ImGui::Separator();
	ImGui::NewLine();
	{
		ImGui::TextUnformatted("ImPlot: 0.12 WIP");
		ImGui::Separator();
		ImGui::Text("By epezent.");
		ImGui::Text("ImPlot is licensed under the MIT License, see LICENSE for more information.");
	}
}

// [... Y X Y X ]
// X: Are tabs open flags
// Y: Are tabs focused flags
void renderer::global_state::ImGui_BrdigeRenderer::LoadTabs(uint32_t serialized_tabs)
{
	for (ptrdiff_t i = 0; i < std::ssize(this->MainTabsInfo); ++i)
	{
		this->MainTabsInfo[i].IsOpen = (serialized_tabs & (1 << (i * 2))) != 0;
		this->MainTabsInfo[i].IsFocused = (serialized_tabs & (1 << (i * 2 + 1))) != 0;
	}
}

void renderer::global_state::ImGui_BrdigeRenderer::SaveTabs(nlohmann::json& out_config)
{
	uint32_t serialized_tabs{ };
	for (ptrdiff_t i = 0; i < std::ssize(this->MainTabsInfo); ++i)
	{
		if (this->MainTabsInfo[i].IsOpen)
			serialized_tabs |= (1 << (i * 2));
		if (this->MainTabsInfo[i].IsFocused)
			serialized_tabs |= (1 << (i * 2 + 1));
	}
	out_config = serialized_tabs;
}

void renderer::global_state::LoadFonts()
{
	ImGuiIO& io = ImGui::GetIO();
	// First is the interface's default font
	{
		ImFontConfig font_cfg;
		font_cfg.OversampleH = 2;

		extern unsigned int Arimo_Medium_compressed_data[];
		extern unsigned int Arimo_Medium_compressed_size;
		io.Fonts->AddFontFromMemoryCompressedTTF(
			Arimo_Medium_compressed_data,
			Arimo_Medium_compressed_size,
			15.f,
			&font_cfg,
			ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()
		);
	}

	// Second is the interface's default icons
	{
		ImFontConfig font_cfg;
		font_cfg.MergeMode = true;

		static const ImWchar icon_ranges[] = {
			ICON_MIN_FA, ICON_MAX_FA,
			0
		};

		extern unsigned int FontAwesomeSolid900_compressed_data[];
		extern unsigned int FontAwesomeSolid900_compressed_size;
		io.Fonts->AddFontFromMemoryCompressedTTF(
			FontAwesomeSolid900_compressed_data,
			FontAwesomeSolid900_compressed_size,
			13.f,
			&font_cfg,
			icon_ranges
		);
	}

	if (std::string path = px::lib_manager.GoToDirectory(px::PlDirType::Main, "Fonts/Fonts.json"); !path.empty())
	{
		std::ifstream config_file(path);
		if (config_file)
		{
			auto fonts{ nlohmann::json::parse(config_file, nullptr, false, true) };
			for (auto font = fonts.begin(); font != fonts.end(); font++)
			{
				if (!font->is_object())
					continue;

				const auto font_path = font->find("path");
				if (font_path == font->end())
					continue;

				const auto font_jcfg = font->find("config");
				ImFontConfig font_cfg;

				if (font_jcfg != font->end() && font_jcfg->is_object())
				{

#define FONT_KEYVALUE(KEY) std::pair{ #KEY, &font_cfg.KEY }

					// booleans:
					for (auto [key, value] : {
						FONT_KEYVALUE(FontDataOwnedByAtlas),
						FONT_KEYVALUE(PixelSnapH),
						FONT_KEYVALUE(MergeMode)
						})
					{
						const auto font_jvalue = font_jcfg->find(key);
						if (font_jvalue != font_jcfg->end() && font_jvalue->is_boolean())
							*value = *font_jvalue;
					}

					// integers:
					for (auto [key, value] : {
						FONT_KEYVALUE(FontNo),
						FONT_KEYVALUE(OversampleH),
						FONT_KEYVALUE(OversampleV)
						})
					{
						const auto font_jvalue = font_jcfg->find(key);
						if (font_jvalue != font_jcfg->end() && font_jvalue->is_number_integer())
							*value = *font_jvalue;
					}

					// unsigned integer:
					{
						auto [key, value] = FONT_KEYVALUE(FontBuilderFlags);
						const auto font_jvalue = font_jcfg->find(key);
						if (font_jvalue != font_jcfg->end() && font_jvalue->is_number_integer())
							*value = *font_jvalue;
					}

					// floats:
					for (auto [key, value] : {
						FONT_KEYVALUE(SizePixels),
						FONT_KEYVALUE(GlyphMinAdvanceX),
						FONT_KEYVALUE(GlyphMaxAdvanceX),
						FONT_KEYVALUE(RasterizerMultiply),
						std::pair{ "GlyphExtraSpacing.x", &font_cfg.GlyphExtraSpacing.x }
						})
					{
						const auto font_jvalue = font_jcfg->find(key);
						if (font_jvalue != font_jcfg->end() && font_jvalue->is_number_float())
							*value = *font_jvalue;
					}

#undef FONT_KEYVALUE

				}

				const auto font_jranges = font->find("ranges");
				std::unique_ptr<ImWchar[]> font_ranges;

				if (font_jranges != font->end())
				{
					// handle array of ranges
					if (font_jranges->is_array() && font_jranges->at(0).is_number_integer())
					{
						font_ranges = std::make_unique<ImWchar[]>(font_jranges->size() + 1);
						size_t pos = 0;
						for (uint32_t offs : *font_jranges)
							font_ranges[pos++] = offs;
						font_ranges[pos] = 0;
					}
					// using built-in ranges
					else if (font_jranges->is_string())
					{

#define RANGE_KEYVALUE(KEY, FUNCTION, SIZE) std::tuple{ KEY, &ImFontAtlas::FUNCTION, SIZE }

						const std::string& range_name = font_jranges->get_ref<const std::string&>();
						for (auto [key, get_range_fn, range_size] : {
							RANGE_KEYVALUE("Default",			GetGlyphRangesDefault, 2),
							RANGE_KEYVALUE("Cyrillic",			GetGlyphRangesCyrillic, 8),
							RANGE_KEYVALUE("Korean",			GetGlyphRangesKorean, 8),
							RANGE_KEYVALUE("Chinese Full",		GetGlyphRangesChineseFull, 14),
							RANGE_KEYVALUE("Thai",				GetGlyphRangesThai, 6),
							RANGE_KEYVALUE("Vietnamese",		GetGlyphRangesVietnamese, 16),
							})
						{
							if (key == range_name)
							{
								font_ranges = std::make_unique<ImWchar[]>(range_size + 1);
								const ImWchar* ranges = (io.Fonts->*get_range_fn)();
								for (int i = 0; i < range_size; i++)
									font_ranges[i] = ranges[i];
								font_ranges[range_size] = 0;
								break;
							}
						}

						if (!font_ranges)
						{
							if (range_name == "Japenese")
							{
								font_ranges = std::make_unique<ImWchar[]>(6009);
								memcpy_s(font_ranges.get(), 6009, io.Fonts->GetGlyphRangesJapanese(), 6009);
							}
							else if (range_name == "Chinese Common")
							{
								font_ranges = std::make_unique<ImWchar[]>(5013);
								memcpy_s(font_ranges.get(), 5013, io.Fonts->GetGlyphRangesChineseSimplifiedCommon(), 5013);
							}
						}

#undef RANGE_KEYVALUE

					}
				}

				const auto font_size = font->find("size");
				ImFont* pFont = io.Fonts->AddFontFromFileTTF(
					font_path->get_ref<const std::string&>().c_str(),
					font_size == font->end() ? 15.f : font_size->get<float>(),
					&font_cfg,
					font_ranges.get()
				);

				if (!pFont)
				{
					PX_LOG_ERROR(
						PX_MESSAGE("Failed to load font"),
						PX_LOGARG("Name", font.key())
					);
					continue;
				}

				px::imgui_iface.m_LoadedFonts[font.key()] = { pFont, std::move(font_ranges) };
			}
		}
	}

	ImGui::GetIO().Fonts->Build();
}
