#pragma once

#include "Defines.hpp"
#include <random>
#include <algorithm>

#ifdef SG_PROFILER_THREADED
#include <mutex>
#endif

SG_NAMESPACE_BEGIN;
SG_BEGIN_PROFILER_NS();

class Manager
{
    friend class Entry;
public:
    static Manager* Get() noexcept
    {
        return Instance;
    }

    static void Set(Manager* ctx) noexcept
    {
        Instance = ctx;
    }

    static Manager* Alloc() noexcept
    {
        return (Instance = new Manager);
    }

    static void Release() noexcept
    {
        delete Instance;
        Instance = nullptr;
    }

    /// <summary>
    /// Turn on/off profiling for current code-space and erase unfinished entries
    /// </summary>
    void Toggle(bool on_or_off);

    /// <summary>
    /// Check if profiling is enabled
    /// </summary>
    /// <returns></returns>
    bool IsEnabled() const noexcept
    {
        return m_IsEnabled;
    }

    /// <summary>
    /// Remove a section from the profiler
    /// </summary>
    /// <param name="section_name">section name or empty string to clear every section</param>
    void ClearSection(const Types::string_t section_name)
    {
        if (!section_name.empty())
            m_Sections.erase(section_name);
        else
            m_Sections.clear();
    }

    /// <summary>
    /// Get profiler's section by name
    /// </summary>
    Types::entry_container* GetSection(const Types::string_t& section_name)
    {
        auto iter = this->m_Sections.find(section_name);
        return iter == this->m_Sections.end() ? nullptr : &iter->second;
    }

    /// <summary>
    /// Get profiler's sections
    /// </summary>
    Types::section_container& GetSections()
    {
        return m_Sections;
    }

    /// <summary>
    /// Erase childrens of 'iter' with same stackoffset from 'container' 
    /// </summary>
    /// <returns>the new position the iterator is poiting to to</returns>
    static Types::entry_container::iterator EraseChildrens(Types::entry_container& container, Types::entry_container::iterator iter);

private:
    /// <summary>
    /// Insert a new entry to the section for profiling
    /// Note: the function will return 'Types::invalid_section_id' if the profiler is not active
    /// </summary>
    /// <param name="color">optional, if it's empty, use a slightly different version of the latest color</param>
    Types::sae_iterator BeginSection(bool backtrace, Types::cstring_t section_name, Types::cstring_t entry_name, const Types::color_type& color = { });

    /// <summary>
    /// Ends an entry timer
    /// </summary>
    void EndSection(const Types::sae_iterator& iter);

    Types::section_container::iterator
        InvalidSection() noexcept
    {
        return m_Sections.end();
    }

public:
    /// <summary>
    /// Entries' garident color
    /// -1 : no color change
    /// </summary>
    float MultColor{ 0.8f };

    /// <summary>
    /// Entry's first default color
    /// </summary>
    Types::color_type Color{ 64, 73, 147, 255 };

    /// <summary>
    /// Stack's depth for 'boost::stacktrack'
    /// </summary>
    size_t StackDepth{ std::numeric_limits<size_t>::max() };

private:
    static inline Manager* Instance = nullptr;

    Types::section_container m_Sections;
#ifdef SG_PROFILER_THREADED
    std::mutex m_Mutex;
    thread_local
#endif
    size_t m_StackOffset{ };

    bool m_IsEnabled{ };
};


inline Types::sae_iterator Manager::BeginSection(bool backtrace, Types::cstring_t section_name, Types::cstring_t entry_name, const Types::color_type& color)
{
    Types::time_point now = Types::clock_type::now();

#ifdef SG_PROFILER_THREADED
    std::lock_guard guard(m_ContextManager->m_Mutex);
#endif

    auto sec_iter = this->m_Sections.emplace(std::piecewise_construct, std::forward_as_tuple(section_name), std::forward_as_tuple(Types::entry_container{ })).first;
    Types::entry_container& entries = sec_iter->second;

    Types::color_type clr;
    if (entries.empty())
    {
        clr = color.rgba[3] ? color : this->Color;
    }
    else if (!this->m_StackOffset)
    {
        clr = color.rgba[3] ? color : this->Color;
    }
    else
    {
        clr = color.rgba[3] ? color : entries.back().color;
        for (auto* c : { &clr.rgba[0], &clr.rgba[1], &clr.rgba[2] })
        {
            using rgba_t = Types::color_type::type;
            *c = static_cast<rgba_t>(static_cast<float>(*c) * this->MultColor);
        }
    }

    auto end = entries.emplace(
        entries.end(),
        now,
        backtrace ? std::make_unique<Types::stacktrace>(boost::stacktrace::stacktrace(4, this->StackDepth)) : nullptr,
        entry_name,
        ++this->m_StackOffset,
        clr
    );

    return Types::sae_iterator{ sec_iter, end };
}


inline void Manager::EndSection(const Types::sae_iterator& iter)
{
    Types::time_point now = Types::clock_type::now();

#ifdef SG_PROFILER_THREADED
    std::lock_guard guard(m_ContextManager->m_Mutex);
#endif

    for (auto sec = this->m_Sections.begin(); sec != this->m_Sections.end(); sec++)
    {
        if (sec == iter.first)
        {
            for (auto entry = sec->second.begin(); entry != sec->second.end(); entry++)
            {
                if (entry == iter.second)
                {
                    entry->end_time = now;
                    break;
                }
            }
            break;
        }
    }

    --this->m_StackOffset;
}


inline void Manager::Toggle(bool on_or_off)
{
#ifdef SG_PROFILER_THREADED
    std::lock_guard guard(m_ContextManager->m_Mutex);
#endif

    m_IsEnabled = on_or_off;
    if (!on_or_off)
    {
        for (auto& [_, entries] : m_Sections)
        {
            for (auto iter = entries.begin(); iter != entries.end(); iter++)
            {
                if (!iter->is_valid())
                {
                    iter = Manager::EraseChildrens(entries, iter);
                    if (iter == entries.end())
                        break;
                }
            }
        }
    }

    m_StackOffset = 0;
}

inline Types::entry_container::iterator Manager::EraseChildrens(Types::entry_container& container, Types::entry_container::iterator iterator)
{
    auto old_pos = iterator;
    const size_t parent_offset = iterator->stackoffset;

    // find it's childrens if they exist
    // and remove them from their parent
    for (++iterator; iterator != container.end(); iterator++)
    {
        if (iterator->stackoffset == parent_offset)
        {
            --iterator;
            break;
        }
    }

    return iterator == old_pos ? container.erase(iterator) : container.erase(old_pos, iterator);
}

SG_END_PROFILER_NS();
SG_NAMESPACE_END;
