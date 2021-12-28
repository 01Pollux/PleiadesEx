
#include <filesystem>
#include <fstream>

#include "Console.hpp"
#include <imgui/imgui_stdlib.h>

#include <regex>

SG_NAMESPACE_BEGIN;

ImGui_Console imgui_console;

void ImGui_BrdigeRenderer::RenderConsole()
{
	imgui_console.Render();
}

void ImGui_Console::Render()
{
	if (ImGui::BeginChild("Console info"))
	{
        auto OnEditTextCallback =
			[](ImGuiInputTextCallbackData* pData) -> int
		{
            switch (pData->EventFlag)
            {
            case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                ptrdiff_t prev_history_pos = imgui_console.m_HistoryPos;
                if (pData->EventKey == ImGuiKey_UpArrow)
                {
                    if (imgui_console.m_HistoryPos == -1)
                        imgui_console.m_HistoryPos = imgui_console.m_HistoryCmds.size() - 1;
                    else if (imgui_console.m_HistoryPos > 0)
                        imgui_console.m_HistoryPos--;
                }
                else if (pData->EventKey == ImGuiKey_DownArrow)
                {
                    if (imgui_console.m_HistoryPos != -1)
                        if (std::cmp_less_equal(++imgui_console.m_HistoryPos, imgui_console.m_HistoryCmds.size()))
                            imgui_console.m_HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != imgui_console.m_HistoryPos)
                {
                    std::string_view history_str = (imgui_console.m_HistoryPos >= 0) ? imgui_console.m_HistoryCmds[imgui_console.m_HistoryPos] : "";
                    pData->DeleteChars(0, pData->BufTextLen);
                    pData->InsertChars(0, history_str.data());
                }
            }
            }
            return 0;
		};

        bool scroll_to_bottom = false;
        bool reset_focus = false;

        if (ImGui::InputText(
            "Input",
            &this->m_Input,
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory,
            OnEditTextCallback
        ))
        {
            reset_focus = true;
            scroll_to_bottom = this->ExecuteCommands();
            this->m_HistoryCmds.emplace_back(this->m_Input);
            this->m_Input.clear();
        }
        ImGui::Separator();

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("ScrollRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (ImGui::BeginPopupContextWindow())
            {
                ImGui::EndPopup();
            }

            if (this->m_Logs.size())
            {
                ImGuiListClipper clipper;
                clipper.Begin(this->m_Logs.size());

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
                {
                    while (clipper.Step())
                    {
                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                        {
                            auto& log_info = this->m_Logs[i];
                            ImGui::PushStyleColor(ImGuiCol_Text, log_info.Color);
                            ImGui::TextUnformatted(log_info.Text.c_str());
                            ImGui::PopStyleColor();
                        }
                    }
                }
                ImGui::PopStyleVar();
            }
        }
        ImGui::EndChild();

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.f);

        ImGui::SetItemDefaultFocus();
        if (reset_focus)
            ImGui::SetKeyboardFocusHere(-1);
		
	}
    ImGui::EndChild();
}


bool ImGui_Console::ExecuteCommands()
{
    std::vector<std::pair<std::string_view, std::string_view>> args;
    SG::ConCommand* pCmd = nullptr;

    auto execute_args = [&args, pCmd, this](const std::string_view& val)
    {
        CommandArgs cmd_args{ std::move(args), val};
        const char* callback_str = pCmd->exec_callback()(
            pCmd,
            cmd_args
        );
        if (callback_str)
        {
            constexpr uint32_t white_clr = 255 | 255 << 0x8 | 255 << 0x10 | 255 << 0x18;
            this->m_Logs.emplace_back(
                white_clr,
                callback_str
            );
        }
    };

    auto end = m_Input.end();
    auto advance_arg = [end](std::string::const_iterator& iter) -> std::string::const_iterator
    {
        bool do_break = false;
        bool skip_next = false;
        bool in_quote = false;
        bool unquote = false;

        auto final_token = iter;
        if (*final_token == '"')
        {
            unquote = in_quote = true;
            final_token = ++iter;
        }

        for (; final_token != end; ++final_token)
        {
            if (skip_next)
                continue;

            switch (*final_token)
            {
            case '\\':
            {
                skip_next = true;
                break;
            }
            case '"':
            {
                in_quote = !in_quote;
                break;
            }
            case ' ':
            {
                do_break = true;
                break;
            }
            }

            if (!in_quote && do_break)
                break;
        }

        if (unquote)
            --final_token;

        return final_token;
    };

    auto begin = m_Input.begin();
    while (begin != end && (*begin == ' ' || *begin == ';'))
        ++begin;

    auto iter = advance_arg(begin);
    // no whitespaces
    if (begin != end)
    {
        std::string_view cmd_name = { begin, iter };
        pCmd = SG::console_manager.FindCommand(cmd_name);
        if (!pCmd)
        {
            constexpr uint32_t red_clr = 255 | 102 << 0x8 | 102 << 0x10 | 255 << 0x18;
            this->m_Logs.emplace_back(
                red_clr,
                std::format("Command '{}' is not a command nor a convar.", cmd_name)
            );
            return true;
        }
    }
    // the command doesnt have any value nor args
    else if (iter == end)
    {
        execute_args("");
        return true;
    }
    else return false;

    // TOODO : execute if the cmd is like cc a; cc aa bb
    // where there is semicolon

    for (; iter != end; iter++)
    {
        if (*iter == '-')
        {
            auto next_iter = ++iter;
            auto arg_name_iter = advance_arg(next_iter);
            std::string_view  arg_name{ next_iter, arg_name_iter };
            auto arg_val_iter = arg_name_iter == end ? end : advance_arg(++arg_name_iter);
            std::string_view  arg_val{ arg_name_iter, arg_val_iter };

            iter = arg_val_iter;
            args.emplace_back(
                arg_name,
                arg_val
            );

            if (iter == end)
                break;
        }
        else if (*iter != ' ')
        {
            auto val_beg = iter;
            bool in_quote = false;

            while (iter != end)
            {
                if (*iter == '"')
                    in_quote = !in_quote;
                else if (*iter == ';' && !in_quote)
                    break;
                ++iter;
            }

            execute_args({ val_beg, iter });

            if (iter != end)
            {
                while (iter != end && (*iter == ' ' || *iter == ';'))
                    ++iter;

                if (iter != end)
                {
                    val_beg = iter;
                    iter = advance_arg(val_beg);

                    if (val_beg != end)
                    {
                        std::string_view cmd_name = { val_beg, iter };
                        pCmd = SG::console_manager.FindCommand(cmd_name);
                        if (!pCmd)
                        {
                            constexpr uint32_t red_clr = 255 | 102 << 0x8 | 102 << 0x10 | 255 << 0x18;
                            this->m_Logs.emplace_back(
                                red_clr,
                                std::format("Command '{}' is not a command nor a convar.", cmd_name)
                            );
                            return true;
                        }
                        continue;
                    }
                }
                break;
            }
            else break;
        }
    }

    return true;
}


SG_NAMESPACE_END;