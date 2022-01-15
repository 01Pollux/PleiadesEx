#include <imgui/imgui_stdlib.h>
#include "Console.hpp"


void ImGui_Console::Render()
{
	static bool scroll_to_bottom = false;

	if (ImGui::BeginChild("Console info"))
	{
		if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Escape)))
		{
			scroll_to_bottom = true;

			this->ClearHistory();
			this->ClearInput();
		}

		ImGui::PushItemWidth(-FLT_MIN);
		bool claim_focus = false;
		if (ImGui::InputText(
			"##Input Commands",
			&this->m_Input,
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

		ImGui::PopItemWidth();

		ImGui::SetItemDefaultFocus();
		if (claim_focus)
			ImGui::SetKeyboardFocusHere(-1);

		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::Separator();

		const float child_size_y = -ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.25f;

		// Draw logs
		if (ImGui::BeginChild(
			"##Scroll-Region",
			{ 0.f, child_size_y },
			false,
			ImGuiWindowFlags_HorizontalScrollbar
		))
		{
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::Selectable("Clear"))
					this->ClearInput();
				if (ImGui::Selectable("Flush"))
				{
					this->ClearHistory();
					this->ClearInput();
				}
				ImGui::EndPopup();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

			if (this->m_Logs.size())
			{
				for (auto& log_info : this->m_Logs)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, log_info.Color);

					ImGui::TextUnformatted(log_info.Text.c_str(), log_info.Text.c_str() + log_info.Text.size());
					ImGui::PopStyleColor();
				}
			}

			ImGui::PopStyleVar();

			if (scroll_to_bottom)
			{
				ImGui::SetScrollHereY(1.f);
				scroll_to_bottom = false;
			}
		}
		ImGui::EndChild();

		if (!this->m_Input.empty())
		{
			ImGui::SetCursorPos(pos);

			constexpr ImU32 color{ 230ul << 0x18 };
			ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
			bool is_open = ImGui::BeginChild("##Suggested Commands", { 0.f, ImGui::GetTextLineHeightWithSpacing() * 5.f }, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
			ImGui::PopStyleColor(2);

			if (is_open)
			{
				//size_t entries = 0;
				for (px::ConCommand* cmd : m_Commands)
				{
					if (this->InputContaints(cmd))
					{
						ImGui::TextUnformatted(cmd->name().data());
						//++entries;
					}
				}

				//entries = std::min(entries, 10u);
				//ImGui::SetWindowSize({ 0.f, ImGui::GetTextLineHeightWithSpacing() * entries });
			}
			ImGui::EndChild();
		}
	}
	ImGui::EndChild();
}

void ImGui_Console::ClearHistory()
{
	this->m_HistoryCmds.clear();
	px::imgui_console.m_HistoryPos = -1;
}

void ImGui_Console::InsertToHistory()
{
	this->m_HistoryCmds.emplace_back(this->m_Input);
	px::imgui_console.m_HistoryPos = -1;
}

bool ImGui_Console::InputContaints(px::ConCommand* cmd)
{
	// cmd::name() = "find"
	// m_Input = "help some_command; fin"
	// m_Input = "fin"

	size_t size = cmd->name().size() - 1;
	bool start_checking = false;

	for (int i = static_cast<int>(m_Input.size()) - 1; i >= 0; i--)
	{
		if (m_Input[i] == ' ')
			return false;

		if (!start_checking)
		{
			if (cmd->name()[size--] != m_Input[i])
				continue;
			start_checking = true;
		}
		else if (cmd->name()[size] != m_Input[i])
			break;
		
		if (!--size)
			return true;
	}
	return false;
}

int ImGui_Console::OnEditTextCallback(ImGuiInputTextCallbackData* pData)
{
	switch (pData->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackHistory:
	{
		ptrdiff_t prev_history_pos = px::imgui_console.m_HistoryPos;
		if (pData->EventKey == ImGuiKey_UpArrow)
		{
			if (px::imgui_console.m_HistoryPos == -1)
				px::imgui_console.m_HistoryPos = px::imgui_console.m_HistoryCmds.size() - 1;
			else if (px::imgui_console.m_HistoryPos > 0)
				px::imgui_console.m_HistoryPos--;
		}
		else if (pData->EventKey == ImGuiKey_DownArrow)
		{
			if (px::imgui_console.m_HistoryPos != -1)
				if (std::cmp_greater_equal(++px::imgui_console.m_HistoryPos, px::imgui_console.m_HistoryCmds.size()))
					px::imgui_console.m_HistoryPos = -1;
		}

		if (prev_history_pos != px::imgui_console.m_HistoryPos)
		{
			pData->DeleteChars(0, pData->BufTextLen);

			if (px::imgui_console.m_HistoryPos >= 0)
			{
				const std::string& str = px::imgui_console.m_HistoryCmds[px::imgui_console.m_HistoryPos];
				pData->InsertChars(0, str.c_str(), str.c_str() + str.size());
			}
		}
		break;
	}
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		/*size_t pos = px::imgui_console.m_Input.find(' ');
		if (pos == std::string::npos)
			pos = 0;
		std::string_view command{ px::imgui_console.m_Input.begin(), px::imgui_console.m_Input.begin() + pos };
		if (!command.empty())
		{

		}*/
		// TODO: Add popup for input to do for wanted commands
		break;
	}
	}
	return 0;
}
