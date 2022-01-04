
#include <algorithm>
#include <set>
#include "Profiler.hpp"

PX_NAMESPACE_BEGIN();

/*
---------------------------------------------------------------------------------
Name        |   Foo::Bar                                                        |
---------------------------------------------------------------------------------
>Calls (N)  |                                                                   |
---------------------------------------------------------------------------------
    |   [0]     |   XXXns   (YYYus) (ZZZms) |  -25.6%      |    Stack trace     |
---------------------------------------------------------------------------------
    |   [1]     |   XXXns   (YYYus) (ZZZms) |  +29.7%      |    Stack trace     |
---------------------------------------------------------------------------------
    |   ...     |   ...nx   (...us) (...ms) |  (1 - this / avg(min/max)) * 100  |
---------------------------------------------------------------------------------
    |   [N-1]   |   XXXns   (YYYus) (ZZZms) |  +...%s      |    Stack trace     |
---------------------------------------------------------------------------------
Min         |   XXXns   (YYYus) (ZZZms)                                         |
---------------------------------------------------------------------------------
Max         |   XXXns   (YYYus) (ZZZms)                                         |
---------------------------------------------------------------------------------
Avg(min/max)|   XXXns   (YYYus) (ZZZms)                                         |
---------------------------------------------------------------------------------
Avg(total)  |   XXXns   (YYYus) (ZZZms)                                         |
---------------------------------------------------------------------------------
*/
void ImGuiProfilerInstance::SectionHandler::DisplaySorted()
{
    enum class SortMode : char
    {
        ByAvgMinMax,
        ByAvgTotal,
        ByMin,
        ByMax
    };

    static constexpr const char* SortNames[]{
        "Avg(min/max)",
        "Avg(total)",
        "Min",
        "Max"
    };

    static_assert(std::ssize(SortNames) == static_cast<size_t>(SortMode::ByMax) + 1);

    static bool with_childrens = true;
    static int num_samples = 50;
    static SortMode sort_mode{ SortMode::ByMax };

    ImGui::PushItemWidth(200.f);
    if (ImGui::BeginCombo("Type", SortNames[static_cast<size_t>(sort_mode)], ImGuiComboFlags_PopupAlignLeft))
    {
        for (int i = 0; i < std::ssize(SortNames); i++)
        {
            const SortMode cur = static_cast<SortMode>(i);
            const bool selected = sort_mode == cur;

            if (ImGui::Selectable(SortNames[i], selected))
                sort_mode = cur;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }  ImGui::SameLine();

    ImGui::Checkbox("With childrens", &with_childrens);  ImGui::SameLine();

    ImGui::SliderInt("Samples", &num_samples, 0, 10'000);

    ImGui::PopItemWidth();

    struct entry_set : std::reference_wrapper<container_type::value_type>
    {
        auto operator<=>(const entry_set& o) const noexcept
        {
            switch (sort_mode)
            {
            case SortMode::ByAvgMinMax:
                return this->get().avg_minmax <=> o.get().avg_minmax;
            case SortMode::ByAvgTotal:
                return this->get().avg_total <=> o.get().avg_total;
            case SortMode::ByMin:
                return this->get().min <=> o.get().min;
            case SortMode::ByMax:
                [[fallthrough]];
            default:
                return this->get().max <=> o.get().max;
            }
        }
    };

    std::multiset<entry_set, std::greater<entry_set>> entries;

    //entries.reserve(this->m_Sections.size());
    for (auto& entry : this->m_Sections)
        entries.emplace(std::ref(entry));

    if (entries.size() > static_cast<size_t>(num_samples) && entries.size())
    {
        entries.erase(
            std::prev(
                entries.rbegin(),
                num_samples - entries.size()
            ).base(),
            entries.end()
        );
    }

    constexpr ImGuiTableFlags table_flags =
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_BordersOuterV |
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_ContextMenuInBody;

    for (auto& entry : entries)
    {
        if (ImGui::TreeNodeEx(entry.get().name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth))
        {
            if (ImGui::BeginTable("Sorted Table", 2, table_flags))
            {
                using namespace std::chrono_literals;
                using name_and_duration = std::pair<const char*, profiler::types::clock_duration>;

                for (auto& name_dur : {
                    name_and_duration{ "Min", entry.get().min },
                    name_and_duration{ "Max", entry.get().max },
                    name_and_duration{ "Avg (min/max)", entry.get().avg_minmax },
                    name_and_duration{ "Avg (total)", entry.get().avg_total },
                })
                {
                    if (ImGui::TableNextColumn())
                        ImGui::TextUnformatted(name_dur.first);
                    if (ImGui::TableNextColumn())
                        ImGui::Text("%lldns (%lldus) (%lldms)", name_dur.second / 1ns, name_dur.second / 1us, name_dur.second / 1ms);
                }

                bool node_is_on = false;
                if (ImGui::TableNextColumn())
                    node_is_on = ImGui::TreeNodeEx("Calls", ImGuiTreeNodeFlags_SpanFullWidth);
                ImGui::TableNextColumn();

                if (node_is_on)
                {
                    ImGuiListClipper clipper;
                    auto& call_stamps = entry.get().entries;
                    clipper.Begin(call_stamps.size());

                    while (clipper.Step())
                    {
                        auto iter = call_stamps.begin();
                        std::advance(iter, clipper.DisplayStart);

                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                        {
                            if (ImGui::TableNextColumn())
                                ImGui::Text("[%i]", i);
                            if (ImGui::TableNextColumn())
                            {
                                auto duration = iter->duration;
                                float pct_minmax = static_cast<float>(duration.count()) / entry.get().avg_minmax.count(),
                                    pct_avg = static_cast<float>(duration.count()) / entry.get().avg_total.count();

                                ImGui::Bullet();
                                ImGui::TextColored(
                                    SectionHandler::GetColor(1.f - pct_minmax),
                                    "%lldns (%lldus) (%lldms)  (Pct min/max: %.3f%%) (Pct avg: %.3f%%)",
                                    duration / 1ns, duration / 1us, duration / 1ms,
                                    100.f - pct_minmax * 100.f,
                                    100.f - pct_avg * 100.f
                                );
                            }
                            ++iter;
                        }
                    }

                    ImGui::TreePop();
                }
                ImGui::EndTable();
            }

            ImGui::TreePop();
        }
    }
}

PX_NAMESPACE_END();
