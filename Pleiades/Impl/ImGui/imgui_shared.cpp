#include <fstream>

#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Interfaces/Logger.hpp"
#include "imgui_iface.hpp"

SG_NAMESPACE_BEGIN;

ImGuiContext* ImGuiInterface::GetContext() noexcept
{
	return m_ImGuiContext;
}

SG::ImGuiPlCallbackId ImGuiInterface::AddCallback(SG::IPlugin* plugin, const char* name, const SG::ImGuiPluginCallback& callback)
{
	auto& callbacks = m_Renderer.PropManager.CallbackProps;

	SG::ImGuiPlCallbackId id = 0;
	for (const auto& cb : callbacks)
	{
		if (cb.second.Id == id)
			++id;
	}
	callbacks.emplace(name, SG::ImGui_BrdigeRenderer::CallbackState{ callback, plugin, id });

	return id;
}

void ImGuiInterface::RemoveCallback(const SG::ImGuiPlCallbackId id)
{
	auto& prop = m_Renderer.PropManager;
	auto iter = std::find_if(prop.CallbackProps.begin(), prop.CallbackProps.end(), [id] (const auto& it) { return it.second.Id == id; });

	if (iter != prop.CallbackProps.end())
	{
		prop.CallbackProps.erase(iter);
		if (prop.Current == iter)
			prop.Current = prop.CallbackProps.end();
	}
}

void ImGuiInterface::RemoveCallback(const SG::IPlugin* plugin) noexcept
{
	auto& prop = m_Renderer.PropManager;
	bool should_invalidate = false;
	for (auto iter = prop.CallbackProps.begin(); iter != prop.CallbackProps.end(); iter++)
	{
		if (iter->second.Plugin != plugin)
			continue;
		prop.CallbackProps.erase(iter);
		if (prop.Current == iter)
			should_invalidate = true;
	}

	if (should_invalidate)
		prop.Current = prop.CallbackProps.end();
}

bool ImGuiInterface::IsMenuOn()
{
	return m_WindowsIsOn;
}

ImFont* ImGuiInterface::FindFont(const char* font_name)
{
	auto iter = m_LoadedFonts.find(font_name);
	return iter != m_LoadedFonts.end() ? iter->second.Font : nullptr;
}


void ImGuiInterface::LoadFonts()
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

	char path[MAX_PATH];
	if (SG::lib_manager.GoToDirectory(SG::PlDirType::Main, "Fonts", path, sizeof(path)))
	{
		std::ifstream config_file(std::string(path) + "/Fonts.json");
		if (config_file)
		{
			nlohmann::json fonts = nlohmann::json::parse(config_file, nullptr, false, true);
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
					SG_LOG_ERROR(
						SG_MESSAGE("Failed to load font"),
						SG_LOGARG("Name", font.key())
					);
					continue;
				}

				m_LoadedFonts[font.key()] = { pFont, std::move(font_ranges) };
			}
		}
	}

	ImGui::GetIO().Fonts->Build();
}

SG_NAMESPACE_END;