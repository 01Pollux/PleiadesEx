
#include <format>
#include "Logger.hpp"


void ImGuiJsLog_HandleDrawPopups(nlohmann::json& info);

void ImGuiJsLog_HandleDrawInfo(const nlohmann::json& info);
void ImGuiJsLog_HandleDrawArray(const nlohmann::json& infoarray);
void ImGuiJsLog_HandleDrawData(const char* key, const nlohmann::json& value);

static std::string ImGuiJsViewPopup;

void ImGuiJsLogInfo::DrawPopupState()
{
	if (imcxx::popup state_popup{ imcxx::popup::context_item{} })
		ImGuiJsLog_HandleDrawPopups(this->Logs);

	if (!ImGuiJsViewPopup.empty())
		ImGui::OpenPopup("Json viewer");

	if (!ImGuiJsViewPopup.empty())
	{
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(.5f, .5f));

		if (imcxx::popup json_viewer_popup{ imcxx::popup::modal{}, "Json viewer", nullptr, ImGuiWindowFlags_NoSavedSettings })
		{
			if (imcxx::window_child json_viewer{ "Json viwer##child", { 720.f, 360.f }, false, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_HorizontalScrollbar })
				ImGui::InputTextMultiline("", ImGuiJsViewPopup.data(), ImGuiJsViewPopup.size(), { -FLT_MIN, -FLT_MIN }, ImGuiInputTextFlags_ReadOnly);

			ImGui::Separator();
			if (ImGui::Button("Exit", { -FLT_MIN, 0.f }))
			{
				ImGuiJsViewPopup.clear();
				ImGuiJsViewPopup.shrink_to_fit();
				json_viewer_popup.close();
			}
		}
	}
}


void ImGuiJsLogInfo::DrawLogs(const ImGuiTextFilter& filter)
{
	auto guess_color =
		[] (const char* c) -> ImVec4
	{
		switch (c[0])
		{
		case 'D': // Debug
			return { 0.84f, 1.0f, 0.07f, 0.1f };
		case 'M': // Message
			return { 1.0f, 0.76f, 0.f, 0.3f };
		case 'E': // Error
			return { 1.0f, 0.25f, 0.16f, 0.41f };
		case 'F': // Fatal
		default:
			return { 1.0f, 0.f, 0.f, 0.51f };
		}
	};
	
	for (auto iter = this->Logs.begin(); iter != this->Logs.end(); iter++)
	{
		if (!this->FilterInfo(filter, iter, true))
			continue;

		ImVec4 clr = guess_color(iter.key().c_str());
		// ImGuiCol_Header : { *, *, *, 0.5f }
		// ImGuiCol_HeaderHovered : { *, *, *, 0.6f }
		// ImGuiCol_HeaderActive : { *, *, *, 0.7f }

		imcxx::shared_color color_override(ImGuiCol_Header, clr);
		clr.w += 0.1f; color_override.push(ImGuiCol_HeaderHovered, clr);
		clr.w += 0.1f; color_override.push(ImGuiCol_HeaderActive, clr);

		// First should sections
		// 'Error', 'Message' etc...
		imcxx::collapsing_header iter_header(iter.key().c_str());
		color_override.pop(color_override.count());


		// Second should be time stamp of the log
		for (auto time_info = iter->begin(); time_info != iter->end(); time_info++)
		{
			if (!this->FilterInfo(filter, time_info, false))
				continue;

			if (iter_header)
			{
				imcxx::tree_node time_node(static_cast<const void*>(std::addressof(*time_info)), ImGuiTreeNodeFlags_SpanFullWidth, "{%s}", time_info.key().c_str());

				bool skip_node = false;
				
				if (imcxx::popup entry_popup{ imcxx::popup::context_item{} })
				{
					ImGuiJsLog_HandleDrawPopups(*time_info);

					if (ImGui::Selectable("Erase"))
					{
						time_info = iter->erase(time_info);
						skip_node = true;
						entry_popup.close();
					}
				}

				if (time_node && !skip_node)
					ImGuiJsLog_HandleDrawInfo(*time_info);
			}
		}
	}
}



void ImGuiJsLog_HandleDrawInfo(const nlohmann::json& info)
{
	using value_t = nlohmann::json::value_t;

	for (auto iter = info.cbegin(); iter != info.cend(); iter++)
	{
		switch (iter->type())
		{
		[[unlikely]]
		case value_t::null:
		{
			break;
		}

		case value_t::object:
		{
			if (imcxx::tree_node draw_tree{ static_cast<const void*>(std::addressof(*iter)), ImGuiTreeNodeFlags_SpanFullWidth, "[%s]", iter.key().c_str()})
				ImGuiJsLog_HandleDrawInfo(*iter);
			break;
		}

		case value_t::array:
		{
			if (imcxx::tree_node draw_tree{ static_cast<const void*>(std::addressof(*iter)), ImGuiTreeNodeFlags_SpanFullWidth, "[%s]", iter.key().c_str() })
				ImGuiJsLog_HandleDrawArray(*iter);
			break;
		}

		[[likely]]
		default:
		{
			ImGuiJsLog_HandleDrawData(iter.key().c_str(), *iter);
			break;
		}
		};
	}
}


void ImGuiJsLog_HandleDrawArray(const nlohmann::json& info)
{
	using value_t = nlohmann::json::value_t;

	char key[8];
	for (size_t arr_pos = 0; const auto& val : info)
	{
		*std::format_to_n(key, std::ssize(key) - 1, "[{}]", arr_pos++).out = 0;

		switch (val.type())
		{
		[[unlikely]]
		case value_t::null:
		{
			break;
		}

		case value_t::object:
		{
			if (imcxx::tree_node draw_tree{ static_cast<const void*>(std::addressof(val)), ImGuiTreeNodeFlags_SpanFullWidth, key})
				ImGuiJsLog_HandleDrawInfo(val);
			break;
		}

		case value_t::array:
		{
			if (imcxx::tree_node draw_tree{ static_cast<const void*>(std::addressof(val)), ImGuiTreeNodeFlags_SpanFullWidth, key })
				ImGuiJsLog_HandleDrawArray(val);
			break;
		}

		[[likely]]
		default:
		{
			ImGuiJsLog_HandleDrawData(&key[3], val);
			break;
		}
		}
	}
}

void ImGuiJsLog_HandleDrawData(const char* key, const nlohmann::json& value)
{
	using value_t = nlohmann::json::value_t;

	if (imcxx::tree_node draw_tree{ static_cast<const void*>(&value), ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth, "%s: ", key})
	{
		ImGui::SameLine(0.f, 10.f);
		switch (value.type())
		{
		[[likely]]
		case value_t::string:
		{
			ImGui::TextWrapped(value.get_ref<const std::string&>().c_str());
			break;
		}
		case value_t::boolean:
		{
			ImGui::TextWrapped(value ? "true" : "false");
			break;
		}
		case value_t::number_integer:
		case value_t::number_unsigned:
		{
			ImGui::TextWrapped("%i", value.get<int>());
			break;
		}
		case value_t::number_float:
		{
			ImGui::TextWrapped("%.2f", value.get<float>());
			break;
		}
		[[unlikely]]
		case value_t::binary:
		{
			const auto& bins = value.get_binary();
			size_t size = bins.size();
			std::string txt("{ ");

			for (size_t i = 0; i < size; i++)
				std::format_to(std::back_inserter(txt), "0x{:#X}", bins[i]);

			if (size)
				std::format_to(std::back_inserter(txt), "0x{:#X} }}", bins[size - 1]);
			else
				txt += "}";

			ImGui::TextWrapped(txt.c_str());
			break;
		}
		}
	}
}


void ImGuiJsLog_HandleDrawPopups(nlohmann::json& info)
{
	if (ImGui::Selectable("Copy"))
	{
		ImGui::LogToClipboard();
		ImGui::LogText(info.dump(4).c_str());
		ImGui::LogFinish();
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::Selectable("Paste"))
	{
		auto pasted{ nlohmann::json::parse(ImGui::GetClipboardText(), nullptr, false, true) };
		if (!pasted.is_discarded())
			info.merge_patch(pasted);
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::Selectable("View"))
	{
		ImGuiJsViewPopup.reserve(1024);
		ImGuiJsViewPopup.assign(info.dump(4));
		ImGui::CloseCurrentPopup();
	}
}
