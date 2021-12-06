#include "Profiler.hpp"
#include "ImPlot/implot.h"

SG_NAMESPACE_BEGIN;

void ImGui_BrdigeRenderer::RenderProfiler()
{
    {
        static bool init = false;
        if (!init)
        {
            ImPlot::SetCurrentContext(ImPlot::CreateContext());
            ImPlot::SetImGuiContext(ImGui::GetCurrentContext());
            init = true;
        }
    }
    
    static ImGuiPlProfiler manager;
    manager.RenderSpace();
    manager.Render();
}


ImGuiPlProfiler::ImGuiPlProfiler()
{
    m_ProfilerInstance.m_Instance = Profiler::Manager::Get();
}

void ImGuiPlProfiler::RenderSpace()
{
    constexpr const char* PopupName = "Color Select";

    if (ImGui::BeginPopup(PopupName))
    {
        ImGui::ColorPicker("##ColorSelect", m_ProfilerInstance.m_Instance->Color.rgba);
        ImGui::EndPopup();
    }

    if (const bool is_on = m_ProfilerInstance.m_Instance->IsEnabled();
        ImGui::Button(is_on ? ICON_FA_PAUSE " Pause" : ICON_FA_PLAY " Resume"))
        m_ProfilerInstance.m_Instance->Toggle(!is_on);

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_REDO " Reload"))
    {
        m_ProfilerInstance.m_Sections.clear();
        m_ProfilerInstance.m_NeedReload = true;
        for (auto& section : m_ProfilerInstance.m_Instance->GetSections())
            m_ProfilerInstance.m_Sections.emplace(section.first, section.second);
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TIMES " Clear"))
    {
        m_ProfilerInstance.erase("");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PALETTE " Color"))
    {
        if (!ImGui::IsPopupOpen(PopupName))
            ImGui::OpenPopup(PopupName);
    }
}

SG_NAMESPACE_END;
