#include "Logger.hpp"
#include <format>

SG_NAMESPACE_BEGIN;

void ImGuiJsLog_HandleDrawPopups(Json& info);

void ImGuiJsLog_HandleDrawInfo(const Json& info);
void ImGuiJsLog_HandleDrawArray(const Json& infoarray);
void ImGuiJsLog_HandleDrawData(const char* key, const Json& value);

static std::string ImGuiJsViewPopup;

void ImGuiJsLogInfo::DrawPopupState()
{
	if (ImGui::BeginPopupContextItem())
	{
		ImGuiJsLog_HandleDrawPopups(this->Logs);
		ImGui::EndPopup();
	}

	if (!ImGuiJsViewPopup.empty())
		ImGui::OpenPopup("Json viewer");

	if (!ImGuiJsViewPopup.empty())
	{
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(.5f, .5f));

		if (ImGui::BeginPopupModal("Json viewer", nullptr, ImGuiWindowFlags_NoSavedSettings))
		{
			if (ImGui::BeginChild("Json viwer##child", { 720.f, 360.f }, false, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_HorizontalScrollbar))
				ImGui::InputTextMultiline("", ImGuiJsViewPopup.data(), ImGuiJsViewPopup.size(), { -FLT_MIN, -FLT_MIN }, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndChild();

			ImGui::Separator();
			if (ImGui::Button("Exit", { -FLT_MIN, 0.f }))
			{
				ImGuiJsViewPopup.clear();
				ImGuiJsViewPopup.shrink_to_fit();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
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
		for (size_t i = 0; i < 4; i++)
		{
			clr.w += 0.1f;
			ImGui::PushStyleColor(ImGuiCol_Header + i, clr);
		}

		// First should sections
		// 'Error', 'Message' etc...
		bool header_on = ImGui::CollapsingHeader(iter.key().c_str());
		ImGui::PopStyleColor(4);


		// Second should be time stamp of the log
		for (auto time_info = iter->begin(); time_info != iter->end(); time_info++)
		{
			if (!this->FilterInfo(filter, time_info, false))
				continue;

			if (header_on)
			{
				bool node_is_on = ImGui::TreeNodeEx(&*time_info, ImGuiTreeNodeFlags_SpanFullWidth, "{%s}", time_info.key().c_str());

				bool skip_node = false;

				if (ImGui::BeginPopupContextItem())
				{
					ImGuiJsLog_HandleDrawPopups(*time_info);

					if (ImGui::Selectable("Erase"))
					{
						time_info = iter->erase(time_info);
						skip_node = true;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (node_is_on)
				{
					if (!skip_node)
						ImGuiJsLog_HandleDrawInfo(*time_info);

					ImGui::TreePop();
				}
			}
		}
	}
}



void ImGuiJsLog_HandleDrawInfo(const Json& info)
{
	using value_t = Json::value_t;

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
			if (ImGui::TreeNodeEx(&*iter, ImGuiTreeNodeFlags_SpanFullWidth, "{%s}", iter.key().c_str()))
			{
				ImGuiJsLog_HandleDrawInfo(*iter);
				ImGui::TreePop();
			}
			break;
		}

		case value_t::array:
		{
			if (ImGui::TreeNodeEx(&*iter, ImGuiTreeNodeFlags_SpanFullWidth, "[%s]", iter.key().c_str()))
			{
				ImGuiJsLog_HandleDrawArray(*iter);
				ImGui::TreePop();
			}
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


void ImGuiJsLog_HandleDrawArray(const Json& info)
{
	using value_t = Json::value_t;

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
			if (ImGui::TreeNodeEx(&val, ImGuiTreeNodeFlags_SpanFullWidth, key))
			{
				ImGuiJsLog_HandleDrawInfo(val);
				ImGui::TreePop();
			}
			break;
		}

		case value_t::array:
		{
			if (ImGui::TreeNodeEx(&val, ImGuiTreeNodeFlags_SpanFullWidth, key))
			{
				ImGuiJsLog_HandleDrawArray(val);
				ImGui::TreePop();
			}
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

void ImGuiJsLog_HandleDrawData(const char* key, const Json& value)
{
	using value_t = Json::value_t;

	if (ImGui::TreeNodeEx(&value, ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth, "%s: ", key))
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
			{
				std::format_to(std::back_inserter(txt), "0x{:#X}", bins[i]);
			}

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



void ImGuiJsLog_HandleDrawPopups(Json& info)
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
		Json pasted = Json::parse(ImGui::GetClipboardText(), nullptr, false, true);
		if (!pasted.is_discarded())
		{
			info.merge_patch(pasted);
		}
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::Selectable("View"))
	{
		ImGuiJsViewPopup.reserve(1024);
		ImGuiJsViewPopup.assign(info.dump(4));
		ImGui::CloseCurrentPopup();
	}
}



SG_NAMESPACE_END;