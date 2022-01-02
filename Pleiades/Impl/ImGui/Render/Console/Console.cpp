#include "Console.hpp"
#include <imgui/imgui_stdlib.h>

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
                        if (std::cmp_greater_equal(++imgui_console.m_HistoryPos, imgui_console.m_HistoryCmds.size()))
                            imgui_console.m_HistoryPos = -1;
                }

                if (prev_history_pos != imgui_console.m_HistoryPos)
                {
                    pData->DeleteChars(0, pData->BufTextLen);

                    if (imgui_console.m_HistoryPos >= 0)
                    {
                        const std::string& str = imgui_console.m_HistoryCmds[imgui_console.m_HistoryPos];
                        pData->InsertChars(0, str.c_str(), str.c_str() + str.size());
                    }
                }
            }
            }
            return 0;
		};

        static bool scroll_to_bottom = false;
        if (scroll_to_bottom)
        {
            ImGui::SetScrollHereY(1.f);
            scroll_to_bottom = false;
        }

        const float footer_height_to_reserve = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 1.25f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
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
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputText(
            "##INPUT",
            &this->m_Input,
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
            OnEditTextCallback
        ))
        {
            scroll_to_bottom = true;

            constexpr uint32_t white_clr = 255 | 255 << 0x8 | 255 << 0x10 | 255 << 0x18;
            SG::console_manager.Print(white_clr, "] " + this->m_Input);

            this->m_HistoryCmds.emplace_back(this->m_Input);
            imgui_console.m_HistoryPos = -1;

            SG::console_manager.Execute(this->m_Input);
            this->m_Input.clear();
        }
        ImGui::PopItemWidth();

        ImGui::SetItemDefaultFocus();
        if (scroll_to_bottom)
            ImGui::SetKeyboardFocusHere(-1);

	}
    ImGui::EndChild();
}


SG_NAMESPACE_END;