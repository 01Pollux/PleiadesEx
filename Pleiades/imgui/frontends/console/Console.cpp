
#include <imgui/imcxx/all_in_one.hpp>
#include "console/Manager.hpp"

void ImGui_Console::Render()
{
	static bool scroll_to_bottom = false;

	if (imcxx::window_child console_info{ "Console info" })
	{
		if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Escape)))
		{
			scroll_to_bottom = true;

			this->ClearHistory();
			this->ClearInput();
		}

		bool claim_focus = false;
		{
			imcxx::shared_item_width override_width(-FLT_MIN);

			if (imcxx::text(
				"##Input Commands",
				this->m_Input,
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
				&ImGui_Console::OnEditTextCallback
			))
			{
				claim_focus = true;
				scroll_to_bottom = true;

				constexpr uint32_t white_clr = 255 | 255 << 0x8 | 255 << 0x10 | 255 << 0x18;
				this->m_Logs.emplace_back(
					white_clr,
					"] " + this->m_Input
				);

				if (!this->m_Input.empty())
				{
					this->InsertToHistory();

					px::console_manager.Execute(this->m_Input);
					ImGui::SetScrollHereY(1.f);

					this->ClearInput();
				}
			}
		}


		ImGui::SetItemDefaultFocus();
		if (claim_focus)
			ImGui::SetKeyboardFocusHere(-1);

		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::Separator();

		const float child_size_y = -ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.25f;

		// Draw logs
		if (imcxx::window_child scroll_region{
			"##Scroll-Region",
			{ 0.f, child_size_y },
			false,
			ImGuiWindowFlags_HorizontalScrollbar
			})
		{
			if (imcxx::popup clear_popup{ imcxx::popup::context_window{} })
			{
				if (ImGui::Selectable("Clear"))
					this->ClearInput();
				if (ImGui::Selectable("Flush"))
				{
					this->ClearHistory();
					this->ClearInput();
				}
			}


			if (this->m_Logs.size())
			{
				imcxx::shared_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 4.f, 1.f });
				for (auto& log_info : this->m_Logs)
				{
					imcxx::shared_color text_clr(ImGuiCol_Text, log_info.Color);
					ImGui::TextUnformatted(log_info.Text.c_str(), log_info.Text.c_str() + log_info.Text.size());
				}
			}

			if (scroll_to_bottom)
			{
				ImGui::SetScrollHereY(1.f);
				scroll_to_bottom = false;
			}
		}

		if (!this->m_Input.empty())
		{
			ImGui::SetCursorPos(pos);

			constexpr ImU32 color{ 230ul << 0x18 };
			imcxx::shared_color bg_color(ImGuiCol_ChildBg, color, ImGuiCol_WindowBg, color);

			std::vector<px::con_command*> cmds;
			{
				for (auto& iter : m_Commands)
				{
					for (px::con_command* cmd : iter.Commands)
					{
						if (this->InputContaints(cmd))
							cmds.push_back(cmd);
					}
				}
			}

			if (imcxx::window_child suggested_commands{
				"##Suggested Commands",
				{ 0.f, ImGui::GetTextLineHeightWithSpacing() * std::min(cmds.size(), 6u) + ImGui::GetStyle().ItemSpacing.y },
				true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
				})
			{
				for (px::con_command* cmd : cmds)
					ImGui::TextUnformatted(cmd->name().data(), cmd->name().data() + cmd->name().size());
			}
		}
	}
}
