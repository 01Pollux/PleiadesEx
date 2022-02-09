
#include "Console.hpp"
#include "imgui/backends/States.hpp"


void renderer::global_state::ImGui_BrdigeRenderer::RenderConsole()
{
	px::imgui_console.Render();
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

bool ImGui_Console::InputContaints(px::con_command* cmd)
{
	// cmd::name() = "find"
	// m_Input = "help some_command; fin"
	// m_Input = "fin"

	size_t i = 0;
	for (auto input_c : m_Input |
		std::views::reverse |
		std::views::take_while([](const char c) { return c != ' ' && c != ';'; }) |
		std::views::reverse
		)
	{
		if (input_c != cmd->name()[i++])
			return false;
	}

	return true;
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
