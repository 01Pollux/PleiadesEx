
#include <array>
#include "ImPlot/implot.h"
#include "ImPlot/implot_internal.h"
#include "Profiler.hpp"


void ImGuiProfilerInstance::DrawPlotBars(entry_container& entries)
{
    using entry_iterator = entry_container::iterator;

    // allow changed for ns to ms to picos
    enum class time_stype_t
    {
        nano,
        micro,
        milli,
        single
    };

    constexpr auto time_types = std::array{
        std::pair{ "Time (ns)", time_stype_t::nano },
        std::pair{ "Time (us)", time_stype_t::micro },
        std::pair{ "Time (ms)", time_stype_t::milli },
        std::pair{ "Time (s)",  time_stype_t::single },
    };

    static time_stype_t current_view = time_stype_t::nano;
    static float plot_spacing{ .15f };
    const int entries_size = entries.size();

    static int plot_max_entries{ 50 };
    ImGui::SliderInt("Max entries", &plot_max_entries, 0, 100);

    static int plor_start_range{ 0 };
    ImGui::SliderInt("Range view", &plor_start_range, 0, std::max(entries_size - plot_max_entries, 0));

    auto entries_begin = entries.begin();
    for (size_t i = plor_start_range; i > 0 && entries_begin != entries.end(); i--)
    {
        ++entries_begin;
    }
    auto entries_end = entries_begin;
    for (size_t i = std::min(entries_size, plot_max_entries); i > 0 && entries_end != entries.end(); i--)
    {
        ++entries_end;
    }

    // First draw our entries in seperate child for drag&drop
    if (imcxx::window_child dnd_child{ "DND Child", { 120, -FLT_MIN } })
    {
        if (imcxx::combo_box time_select{ "##TIME", time_types[static_cast<size_t>(current_view)].first, ImGuiComboFlags_NoArrowButton })
        {
            for (auto& type : time_types)
            {
                if (ImGui::Selectable(type.first, type.second == current_view))
                    current_view = type.second;
            }
        }

        imcxx::drag::call("##Spacing", &plot_spacing, .1f, 0.f, 0.f, "%.4f");

        for (auto entry = entries_begin; entry != entries_end; ++entry)
        {
            // Don't allow active graphs to the drag&drop list
            {
                imcxx::shared_item_id entry_id(std::addressof(*entry));
                ImPlot::ItemIcon(entry->color);
                ImGui::SameLine();
                ImGui::Selectable(entry->name.c_str(), false, ImGuiSelectableFlags_None, { 100.f, 0.f });
            }

            if (imcxx::popup stack_popup{ imcxx::popup::context_item{} })
            {
                if (ImGui::Selectable("Stack Trace"))
                {
                    if (entry->has_backtrace())
                        ImGuiPlProfiler::StackTracePopup.SetPopupInfo(*entry->stack_info, &entries, &entry);
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::Selectable("Delete"))
                {
                    entry = px::profiler::manager::EraseChildrens(entries, entry);
                    ImGui::CloseCurrentPopup();
                }

                if (entry == entries.end())
                    break;
            }
        }
    }

    ImGui::SameLine();

    {
        struct safe_plot
        {
            safe_plot(const char* time_view) : 
                is_open(ImPlot::BeginPlot(
                    "##PLOT", 
                    "Calls", 
                    time_view,
                    { -FLT_MIN, -FLT_MIN }, 
                    ImPlotFlags_None, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel
                ))
            {}

            ~safe_plot()
            {
                if (is_open)
                    ImPlot::EndPlot();
            }

            constexpr operator bool() const noexcept
            {
                return is_open;
            }

            bool is_open : 1;
        };

        if (safe_plot plot_draw{ time_types[static_cast<size_t>(current_view)].first })
        {
            using namespace std::chrono_literals;
            struct data_t
            {
                long long time;
                double offset{ };
            };
            data_t draw_offset;

            time_stype_t type = time_types[static_cast<size_t>(current_view)].second;
            std::vector<ImPlotPoint> plotlines{ };
            plotlines.reserve(entries.size() / 2);

            size_t pos = 0;
            for (auto entry = entries_begin; entry != entries_end; ++entry)
            {
                switch (type)
                {
                case time_stype_t::nano:
                    draw_offset.time = (entry->end_time - entry->begin_time) / 1ns;
                    break;
                case time_stype_t::micro:
                    draw_offset.time = (entry->end_time - entry->begin_time) / 1us;
                    break;
                case time_stype_t::milli:
                    draw_offset.time = (entry->end_time - entry->begin_time) / 1ms;
                    break;
                case time_stype_t::single:
                    draw_offset.time = (entry->end_time - entry->begin_time) / 1s;
                    break;
                }

                std::string name = std::format("#{}: {}", pos++, entry->name);

                ImPlot::PushStyleColor(ImPlotCol_Fill, entry->color);
                ImPlot::PlotBarsG(
                    name.c_str(),
                    [] (void* pData, int) -> ImPlotPoint
                    {
                        data_t& data = *static_cast<data_t*>(pData);
                        return ImPlotPoint{ data.offset, static_cast<double>(data.time) };
                    },
                    &draw_offset,
                    1,
                    0.1
                );

                ImPlot::PopStyleColor();

                auto curPlot = ImPlot::GetCurrentPlot()->Items.GetItem(name.c_str());
                if (curPlot->Show)
                    plotlines.emplace_back(draw_offset.offset, static_cast<double>(draw_offset.time));

                if (curPlot->LegendHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (entry->has_backtrace())
                        ImGuiPlProfiler::StackTracePopup.SetPopupInfo(*entry->stack_info, &entries, &entry);
                }

                draw_offset.offset += plot_spacing;
            }

            ImPlot::SetNextLineStyle({ 0.f, .9f, 0.f, 1.f }, 2.8f);
            if (plotlines.size() > 1)
            {
                ImPlot::PlotLineG(
                    "##BezQuad",
                    [] (void* pData, int index) -> ImPlotPoint
                    {
                        const int
                            idx = index / 100,
                            pos_in_100 = index % 100;

                        ImPlotPoint&
                            src = static_cast<ImPlotPoint*>(pData)[idx],
                            dst = static_cast<ImPlotPoint*>(pData)[idx + 1];

                        ImPlotPoint ctrl;

                        if (src.y <= dst.y)
                        {
                            ctrl = {
                                dst.x,
                                src.y
                            };
                        }
                        else
                        {
                            ctrl = {
                                src.x,
                                dst.y
                            };
                        }

                        const double
                            t = pos_in_100 / 99.,
                            u = (1 - t),
                            squ = u * u;

                        return {
                            ctrl.x + (squ * (src.x - ctrl.x)) + (t * t * (dst.x - ctrl.x)),
                            ctrl.y + (squ * (src.y - ctrl.y)) + (t * t * (dst.y - ctrl.y))
                        };
                    },
                    plotlines.data(),
                    (plotlines.size() - 1) * 100
                );
            }
        }
    }
}
