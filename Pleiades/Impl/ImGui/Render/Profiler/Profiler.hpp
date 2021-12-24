#pragma once

#include <shadowgarden/users/Profiler.hpp>
#include "../../Render/render.hpp"

SG_NAMESPACE_BEGIN;

struct ImGuiProfilerInstance
{
    using entry_container = Profiler::Types::entry_container;

    enum class draw_type : uint8_t
    {
        PlotBars,
        Hierachy,
        Sorted
    };

    /// <summary>
    /// Clear or erase section
    /// </summary>
    /// <param name="section_name">section name or empty string to clear every section</param>
    void erase(const Profiler::Types::string_t name)
    {
        m_Instance->ClearSection(name);
    }

    struct SectionHandler
    {
        struct entry_info
        {
            const Profiler::Types::clock_duration duration;
            const std::unique_ptr<Profiler::Types::stacktrace>& stack_info;
        };

        struct entry_container
        {
            size_t stack_offset;
            size_t count{ };

            std::list<entry_info> entries;
            const Profiler::Types::string_t& name;
            Profiler::Types::clock_duration min{ Profiler::Types::clock_duration::max() }, max{ Profiler::Types::clock_duration::min() }, avg_minmax{ }, avg_total{ };

            entry_container(size_t stack_offset, const Profiler::Types::string_t& name) :
                stack_offset(stack_offset), name(name)
            { }
        };
        using container_type = std::vector<entry_container>;

        void Update(const Profiler::Types::entry_container& infos);

        void Clear() noexcept
        {
            m_Sections.clear();
        }

        bool Empty() const noexcept
        {
            return m_Sections.empty();
        }

        /// <summary>
        /// Render as a hierachy
        /// </summary>
        void DisplayHierachy();

        /// <summary>
        /// Render as a sorted
        /// </summary>
        void DisplaySorted();

        /// <summary>
        /// Export section to json file
        /// </summary>
        void Export(const std::string& file_name);

        /// <summary>
        /// Get color from ratio [green, red] for hierachy and sorted graph
        /// </summary>
        static ImVec4 GetColor(float ratio)
        {
            if (ratio >= 0.40f)
                return {
                0.f,
                std::clamp(ratio * (405.f / 255.f), 0.f, 1.f),
                0.f,
                1.f
            };
            else if (ratio > 0.f)
                return {
                1.f,
                std::clamp(ratio * (830.f / 255.f), 0.f, 0.8f),
                0.f,
                1.f
            };
            else
                return {
                1.f,
                std::clamp(0.23f - (-ratio * (393.f / 255.f)), 0.f, 0.23f),
                0.f,
                1.f
            };
        }

    private:
        using entry_iterator = ImGuiProfilerInstance::entry_container::iterator;
        using entry_citerator = ImGuiProfilerInstance::entry_container::const_iterator;

        auto FindOrEmplaceWithinOffset(size_t stack_offset, const Profiler::Types::string_t& name);
        
        size_t InsertNestedEntries(entry_citerator cur_iter, const entry_citerator& end_iter, size_t stack_offset = 1);
        void SetElapsedTime() noexcept;

        container_type m_Sections;
    };

    struct section_info
    {
        entry_container entries;
        SectionHandler section_handler;
    };

    /// <summary>
    /// Render as a plot bar
    /// </summary>
    void DrawPlotBars(entry_container&);


    Profiler::Manager* m_Instance{ };
    std::map<std::string, section_info> m_Sections;
    bool m_NeedReload;

    draw_type m_DrawType{ draw_type::Hierachy };
};


class ImGuiPlProfiler
{
public:
    ImGuiPlProfiler();

    /// <summary>
    /// Render profiler's space and draw menu and tools
    /// Menu:
    ///     Tools:
    ///     > Graphs: View current loaded instances' graphs
    ///     > Statistiques: View current loaded instances' statistiques in tables
    /// 
    ///     More:
    ///     > Save: Save main's profiler information to a file
    ///     > Load: Load profiler's information from a file by creating a new profiler instance
    ///     > Reload: Reload main's profiler information
    ///     > Clear: Clear main's profiler instance
    ///     > Resume/Pause: Resume/Pause the main's profiler instance
    /// </summary>
    void RenderSpace();

    void Render();

    class StackTracePopup_t
    {
    public:
        /// <summary>
        /// Display a global popup for stcktrace
        /// </summary>
        /// <param name="stacktrace">stacktrace info from boost::stacktrace::stacktrace</param>
        /// <param name="entries">pointer to entries if the entry is erasable</param>
        /// <param name="current_entry">pointer to to current entry if it's erables</param>
        void SetPopupInfo(
            const Profiler::Types::stacktrace& stacktrace,
            ImGuiProfilerInstance::entry_container* entries,
            ImGuiProfilerInstance::entry_container::iterator* current_entry
        );
        void DisplayPopupInfo();

    private:
        std::string m_StackTrace;
        ImGuiProfilerInstance::entry_container* m_Entries;
        ImGuiProfilerInstance::entry_container::iterator m_CurrentEntry;

        ImGuiID m_PopupId;
    };

    static inline StackTracePopup_t StackTracePopup;

private:
    /*using map_type = std::map<std::string, ImGuiProfilerInstance>;
    map_type m_ProfilerInstances;*/
    ImGuiProfilerInstance m_ProfilerInstance;
};

SG_NAMESPACE_END;
