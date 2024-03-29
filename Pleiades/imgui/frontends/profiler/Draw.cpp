
#include "Profiler.hpp"

void ImGuiPlProfiler::Render()
{
    ImGuiPlProfiler::StackTracePopup.DisplayPopupInfo();

    if (imcxx::tabbar main_profiler_tab{ "Main Profiler", ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_Reorderable })
    {
        for (auto& [section, info] : m_ProfilerInstance.m_Sections)
        {
            if (m_ProfilerInstance.m_NeedReload)
                info.section_handler.Update(info.entries);

            if (auto cur_section = main_profiler_tab.add_item(section))
            {
                using draw_type = ImGuiProfilerInstance::draw_type;
                if (imcxx::popup section_popup{ imcxx::popup::context_item{} })
                {
                    if (ImGui::Selectable(ICON_FA_FILE_EXPORT " Export"))
                    {
                        info.section_handler.Export(section);
                        section_popup.close();
                    }

                    if (ImGui::Selectable(ICON_FA_REDO " Update"))
                    {
                        info.section_handler.Update(info.entries);
                        section_popup.close();
                    }

                    if (ImGui::Selectable(ICON_FA_TIMES " Clear"))
                    {
                        info.section_handler.Clear();
                        section_popup.close();
                    }

                    if (ImGui::Selectable(ICON_FA_TIMES_CIRCLE " Erase"))
                    {
                        m_ProfilerInstance.erase(section);
                        info.section_handler.Clear();
                        section_popup.close();
                    }

                    if (ImGui::RadioButton(ICON_FA_CHART_BAR " Plot", m_ProfilerInstance.m_DrawType == draw_type::PlotBars))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::PlotBars;
                        section_popup.close();
                    }

                    ImGui::SameLine();
                    if (ImGui::RadioButton(ICON_FA_TABLE " Hierachy", m_ProfilerInstance.m_DrawType == draw_type::Hierachy))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::Hierachy;
                        section_popup.close();
                    }

                    ImGui::SameLine();
                    if (ImGui::RadioButton(ICON_FA_TABLE " Sorted", m_ProfilerInstance.m_DrawType == draw_type::Sorted))
                    {
                        m_ProfilerInstance.m_DrawType = draw_type::Sorted;
                        section_popup.close();
                    }
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
            }
        }
        m_ProfilerInstance.m_NeedReload = false;
    }
}


void ImGuiPlProfiler::StackTracePopup_t::SetPopupInfo(
    const px::profiler::types::stacktrace& stacktrace,
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

    if (imcxx::popup stack_strace_popup{ imcxx::popup::modal{}, "##StackTrace", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiPopupFlags_AnyPopupLevel })
    {
        bool eraseable = m_Entries != nullptr;

        const ImVec2 space_size = { (ImGui::GetContentRegionAvail().x / (eraseable ? 3 : 2)) - ImGui::GetStyle().ItemSpacing.x, 0.f };

        ImGui::TextUnformatted(m_StackTrace.c_str(), m_StackTrace.c_str() + m_StackTrace.size());
        ImGui::Separator();

        if (ImGui::Button("Copy", space_size))
            ImGui::SetClipboardText(m_StackTrace.c_str());
        ImGui::SameLine();

        if (eraseable)
        {
            if (ImGui::Button("Delete", space_size))
            {
                px::profiler::manager::EraseChildrens(*m_Entries, m_CurrentEntry);
                m_StackTrace.clear();
                stack_strace_popup.close();
            }
            ImGui::SameLine();
        }

        if (ImGui::Button("Close", space_size))
        {
            m_StackTrace.clear();
            stack_strace_popup.close();
        }
    }
}
