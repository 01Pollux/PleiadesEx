
#include "Profiler.hpp"

/*
-------------------------------------------------------------------------------------------------------------------------------------
Functions               |   Count   |   Min                 |       Max             |   Avg(min/max)        |   Avg(total)          |
-------------------------------------------------------------------------------------------------------------------------------------
    main                |   XXX     |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |
-------------------------------------------------------------------------------------------------------------------------------------
|   Foo::Bar            |   XXX     |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |
-------------------------------------------------------------------------------------------------------------------------------------
|   |   Foo::Bar2       |   XXX     |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |
-------------------------------------------------------------------------------------------------------------------------------------
|   |   |  Foo::BarD    |   XXX     |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |
-------------------------------------------------------------------------------------------------------------------------------------
|   Foo::BarD           |   XXX     |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |   XXns (YYus) (ZZms)  |
-------------------------------------------------------------------------------------------------------------------------------------
*/
static constexpr const char* HierachyNames[]{
    //"Functions"   // 1
    "Count",        // 2
    "Min",          // 3
    "Max",          // 4
    "Avg (min/max)",// 5
    "Avg (total)"   // 6
};

static void ImGuiProfiler_ImplDisplayHierachy(
    ImGuiProfilerInstance::SectionHandler::container_type::iterator cur_pos,
    ImGuiProfilerInstance::SectionHandler::container_type::const_iterator end_pos,
    size_t offset = 1
);


void ImGuiProfilerInstance::SectionHandler::DisplayHierachy()
{
    constexpr ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ContextMenuInBody |
        ImGuiTableFlags_NoHostExtendX;

    if (imcxx::table hierachy_table{ "Hierachy Table", 6, table_flags })
    {
        ImGui::TableSetupColumn("Functions");   // 1
        for (auto sec : HierachyNames)
            ImGui::TableSetupColumn(sec);
        ImGui::TableHeadersRow();
        
        ImGuiProfiler_ImplDisplayHierachy(this->m_Sections.begin(), this->m_Sections.end());
    }
}


void ImGuiProfiler_ImplDisplayHierachy(
    ImGuiProfilerInstance::SectionHandler::container_type::iterator cur_pos,
    ImGuiProfilerInstance::SectionHandler::container_type::const_iterator end_pos,
    size_t offset
)
{
    auto display_infos = [](ImGuiProfilerInstance::SectionHandler::container_type::value_type& entry_info)
    {
        if (imcxx::popup hierachy_popup{ imcxx::popup::context_item{} })
        {
            size_t i = 0;
            auto& entries = entry_info.entries;
            for (auto entry = entries.begin(); entry != entries.end(); entry++)
            {
                if (imcxx::menubar_item menu_entry{ std::format("[{}]", i++) })
                {
                    using namespace std::chrono_literals;

                    bool should_break = false;

                    if (menu_entry.add_entry("Stack Trace"))
                    {
                        if (entry->stack_info)
                        {
                            hierachy_popup.close();
                            ImGuiPlProfiler::StackTracePopup.SetPopupInfo(*entry->stack_info, nullptr, nullptr);
                        }
                        should_break = true;
                    }

                    float pct_minmax = static_cast<float>(entry->duration.count()) / entry_info.avg_minmax.count(),
                        pct_avg = static_cast<float>(entry->duration.count()) / entry_info.avg_total.count();

                    imcxx::shared_color text_color(
                        ImGuiCol_Text,
                        ImGuiProfilerInstance::SectionHandler::GetColor(1.f - pct_minmax)
                    );

                    if (menu_entry.add_entry(
                        std::format(
                            "{}ns ({}us) ({}ms) (Pct min/max: {:.3f}%) (Pct avg: {:.3f}%)",
                            entry->duration / 1ns, entry->duration / 1us, entry->duration / 1ms,
                            100.f - pct_minmax * 100.f,
                            100.f - pct_avg * 100.f
                        ).c_str()))
                    {
                        should_break = true;
                    }

                    if (should_break)
                        break;
                }
            }
        }

        // Count
        if (ImGui::TableNextColumn())
            ImGui::Text("%i", entry_info.count);
        // Min
        // Max
        // Avg (min/max)
        // Avg (total)
        for (auto dur : {
            entry_info.min,
            entry_info.max,
            entry_info.avg_minmax,
            entry_info.avg_total
             })
        {
            using namespace std::chrono_literals;
            if (ImGui::TableNextColumn())
                ImGui::Text("%lldns (%lldus) (%lldms)", dur / 1ns, dur / 1us, dur / 1ms);
        }
    };

    for (; cur_pos != end_pos; ++cur_pos)
    {
        auto next_iter = std::next(cur_pos);
        // check if we're accessing a new section
        if (next_iter != end_pos && next_iter->stack_offset > offset)
        {
            ImGui::TableNextColumn();

            std::string unique_name = std::format("{}##{}", cur_pos->name, static_cast<void*>(&*cur_pos));
            imcxx::tree_node hierachy_node(unique_name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);

            display_infos(*cur_pos);

            // find the next entry with same offset to display them recursively if they are open
            // vvvvvvvvvvvvvvvvvvvvvvvv
            // offset = XX
            //      offset = XX + 1 
            //          offset = XX + 2 
            //          offset = XX + 2 
            //      offset = XX + 1 
            //          offset = XX + 2 
            //      offset = XX + 2
            // ^^^^^^^^^^^^^^^^^^^^^^^^
            // offset = XX + 1 
            for (cur_pos = next_iter, ++cur_pos; cur_pos != end_pos; ++cur_pos)
            {
                if (cur_pos->stack_offset == offset)
                    break;
            }


            // display the section [next_iter, cur_iter(
            if (hierachy_node)
                ImGuiProfiler_ImplDisplayHierachy(next_iter, cur_pos, offset + 1);
            --cur_pos;

            // decrement the cur_iter iterator and check if we've reached end of sections
            if (cur_pos == end_pos)
                return;
        }
        // else we're in the same section
        else
        {
            ImGui::TableNextColumn();
            [[maybe_unused]] imcxx::tree_node entry_node(
                static_cast<const void*>(std::addressof(cur_pos->entries)),
                ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth, 
                "%s",
                cur_pos->name.c_str()
            );
            display_infos(*cur_pos);
        }
    }
}
