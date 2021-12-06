
#include "Profiler.hpp"

SG_NAMESPACE_BEGIN;

void ImGuiPlProfiler::Render()
{
    ImGuiPlProfiler::StackTracePopup.DisplayPopupInfo();

    if (ImGui::BeginTabBar("Main Profiler", ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_Reorderable))
    {
        for (auto& [section, info] : m_ProfilerInstance.m_Sections)
        {
            if (m_ProfilerInstance.m_NeedReload)
                info.section_handler.Update(info.entries);

            if (ImGui::BeginTabItem(section.c_str()))
            {
                using draw_type = ImGuiProfilerInstance::draw_type;
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Selectable(ICON_FA_FILE_EXPORT " Export"))
                    {
                        info.section_handler.Export(section);
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::Selectable(ICON_FA_REDO " Update"))
                    {
                        info.section_handler.Update(info.entries);
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::Selectable(ICON_FA_TIMES " Clear"))
                    {
                        info.section_handler.Clear();
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::Selectable(ICON_FA_TIMES_CIRCLE " Erase"))
                    {
                        m_ProfilerInstance.erase(section);
                        info.section_handler.Clear();
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::RadioButton(ICON_FA_CHART_BAR " Plot", m_ProfilerInstance.m_DrawType == draw_type::PlotBars))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::PlotBars;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(ICON_FA_TABLE " Hierachy", m_ProfilerInstance.m_DrawType == draw_type::Hierachy))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::Hierachy;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton(ICON_FA_TABLE " Sorted", m_ProfilerInstance.m_DrawType == draw_type::Sorted))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::Sorted;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                if (!info.section_handler.Empty())
                {
                    switch (m_ProfilerInstance.m_DrawType)
                    {
                    case draw_type::PlotBars:
                    {
                        m_ProfilerInstance.DrawPlotBars(info.entries);
                        break;
                    }
                    case draw_type::Hierachy:
                    {
                        info.section_handler.DisplayHierachy();
                        break;
                    }
                    case draw_type::Sorted:
                    {
                        info.section_handler.DisplaySorted();
                        break;
                    }
                    }
                }

                ImGui::EndTabItem();
            }
        }

        m_ProfilerInstance.m_NeedReload = false;
        ImGui::EndTabBar();
    }
}


void ImGuiPlProfiler::StackTracePopup_t::SetPopupInfo(
    const Profiler::Types::stacktrace& stacktrace,
    ImGuiProfilerInstance::entry_container* entries,
    ImGuiProfilerInstance::entry_container::iterator* current_entry
)
{
    if (!m_StackTrace.empty())
        return;

    m_StackTrace.assign(boost::stacktrace::to_string(stacktrace));
    m_Entries = entries;
    if (current_entry)
        m_CurrentEntry = *current_entry;

    ImGui::OpenPopup(m_PopupId);
}

void ImGuiPlProfiler::StackTracePopup_t::DisplayPopupInfo()
{
    m_PopupId = ImGui::GetID("##StackTrace");

    if (m_StackTrace.empty())
        return;

    if (ImGui::BeginPopupModal("##StackTrace", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiPopupFlags_AnyPopupLevel))
    {
        bool eraseable = m_Entries != nullptr;

        const ImVec2 space_size = { (ImGui::GetContentRegionAvail().x / (eraseable ? 3 : 2)) - ImGui::GetStyle().ItemSpacing.x, 0.f };

        ImGui::TextUnformatted(m_StackTrace.c_str(), m_StackTrace.c_str() + m_StackTrace.size());
        ImGui::Separator();

        if (ImGui::Button("Copy", space_size))
        {
            ImGui::SetClipboardText(m_StackTrace.c_str());
        }
        ImGui::SameLine();

        if (eraseable)
        {
            if (ImGui::Button("Delete", space_size))
            {
                Profiler::Manager::EraseChildrens(*m_Entries, m_CurrentEntry);
                m_StackTrace.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
        }

        if (ImGui::Button("Close", space_size))
        {
            m_StackTrace.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

SG_NAMESPACE_END;
