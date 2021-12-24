#pragma once

#include "Profiler/Defines.hpp"
#include "Profiler/Context.hpp"

SG_NAMESPACE_BEGIN;
SG_BEGIN_PROFILER_NS();

// Macro used for profiling a section, optionally sets color
#define SG_PROFILE_SECTION(SECTION, ...)            SG::Profiler::Entry profile_##__LINE__{ false, SECTION, __VA_ARGS__ }
#define SG_PROFILE_SECTION_BACKTRACE(SECTION, ...)  SG::Profiler::Entry profile_##__LINE__{ true, SECTION, __VA_ARGS__ }

// Macro used setting/reseting default color
#define SG_PROFILER_PUSH_COLOR(COLOR, IDX)                                  \
    do {                                                                    \
    auto profiler_backup_clr##IDX = SG::Profiler::Manager::Get()->Color;    \
    SG::Profiler::Manager::Get()->Color = COLOR

#define SG_PROFILER_POP_COLOR(IDX)                                          \
    SG::Profiler::Manager::Get()->Color = profiler_backup_clr##IDX;         \
} while (false)

class Entry
{
public:
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;

    Entry(bool backtrace, Types::cstring_t section_name, Types::cstring_t entry_name, const Types::color_type& color = { }) :
        m_SectionIter{ Manager::Get()->IsEnabled() ? Manager::Get()->BeginSection(backtrace, section_name, entry_name, color) : Types::sae_iterator{ Manager::Get()->InvalidSection(), { } } }
    { }

    ~Entry()
    {
        if (is_active())
            Manager::Get()->EndSection(m_SectionIter);
    }

    bool is_active() const noexcept
    {
        return m_SectionIter.first != Manager::Get()->InvalidSection();
    }

private:
    Types::sae_iterator m_SectionIter{ };
};

SG_END_PROFILER_NS();
SG_NAMESPACE_END;
