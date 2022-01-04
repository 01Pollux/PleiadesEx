
#include <fstream>
#include <regex>

#include <nlohmann/Json.hpp>
#include <px/string.hpp>

#include "Impl/Library/LibrarySys.hpp"
#include "Profiler.hpp"

PX_NAMESPACE_BEGIN();

void ImGuiProfilerInstance::SectionHandler::Update(const profiler::types::entry_container& infos)
{
    this->Clear();
    m_Sections.reserve(infos.size());

    InsertNestedEntries(infos.begin(), infos.end());
    SetElapsedTime();

    m_Sections.shrink_to_fit();
}


auto ImGuiProfilerInstance::SectionHandler::FindOrEmplaceWithinOffset(size_t stack_offset, const std::string& name)
{
    for (auto iter = m_Sections.rbegin(); iter != m_Sections.rend(); iter++)
    {
        if (iter->stack_offset != stack_offset)
            continue;
        else if (iter->name == name)
            return --iter.base();
    }

    m_Sections.emplace_back(stack_offset, name);
    return std::prev(m_Sections.end());
}

size_t ImGuiProfilerInstance::SectionHandler::InsertNestedEntries(entry_citerator cur_iter, const entry_citerator& end_iter, size_t stack_offset)
{
    size_t func_count = 0;

    for (; cur_iter != end_iter; ++cur_iter)
    {
        auto next_iter = std::next(cur_iter);
        if (next_iter != end_iter && next_iter->stackoffset > stack_offset)
        {
            auto this_section = FindOrEmplaceWithinOffset(stack_offset, cur_iter->name);

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

            this_section->entries.emplace_back(
                (cur_iter->end_time - cur_iter->begin_time),
                cur_iter->stack_info
            );

            for (cur_iter = next_iter, ++cur_iter; cur_iter != end_iter; ++cur_iter)
            {
                if (cur_iter->stackoffset == stack_offset)
                    break;
            }

            size_t count = InsertNestedEntries(next_iter, cur_iter, stack_offset + 1);
            this_section->count += count;
            func_count += count;
            --cur_iter;
        }
        else
        {
            auto this_section = FindOrEmplaceWithinOffset(stack_offset, cur_iter->name);
            ++this_section->count;

            this_section->entries.emplace_back(
                (cur_iter->end_time - cur_iter->begin_time),
                cur_iter->stack_info
            );

            ++func_count;
        }
    }

    return func_count;
}

void ImGuiProfilerInstance::SectionHandler::SetElapsedTime() noexcept
{
    using namespace std::chrono_literals;

    for (auto& sec : m_Sections)
    {
        for (auto& entry : sec.entries)
        {
            if (sec.min > entry.duration)
                sec.min = entry.duration;

            if (sec.max < entry.duration)
                sec.max = entry.duration;

            sec.avg_total += entry.duration;
        }

        sec.avg_total /= sec.entries.size();
        sec.avg_minmax = (sec.max + sec.max) / 2;
    }
}

void ImGuiProfilerInstance::SectionHandler::Export(const std::string& file_name)
{
    using namespace std::chrono_literals;
    using name_and_duration = std::pair<const char*, profiler::types::clock_duration>;

    char path[MAX_PATH];
    if (!px::lib_manager.GoToDirectory(PlDirType::Profiler, nullptr, path, std::ssize(path)))
        return;

    nlohmann::json data;
    for (auto& section : this->m_Sections)
    {
        auto& cur_sec = data[section.name];
        for (auto& name_dur : {
            name_and_duration{ "min", section.min },
            name_and_duration{ "max", section.max },
            name_and_duration{ "avg (min/max)", section.avg_minmax },
            name_and_duration{ "avg (total)", section.avg_total },
        })
        {
            cur_sec[name_dur.first] = std::format("{}ns ({}us) ({}ms)", name_dur.second / 1ns, name_dur.second / 1us, name_dur.second / 1ms);
        }

        auto& entries = cur_sec["Entries"];
        size_t stacktrace_index = 0;

        for (auto& entry : section.entries)
        {
            entries["time"] = std::format("{}ns ({}us) ({}ms)", entry.duration / 1ns, entry.duration / 1us, entry.duration / 1ms);
            if (entry.stack_info)
            {
                // it's very slow if we were to iterate through boost::stacktrace::frame(s) and get name, source file and line
                // instad we will dump everything to a single string and split it by new line
                std::string stacktrace = boost::stacktrace::to_string(*entry.stack_info);
                std::regex token{ "\\n+" };
                std::sregex_token_iterator iter{ stacktrace.cbegin(), stacktrace.end(), token, -1 }, end;
                auto& trace = entries["stack trace"][stacktrace_index++];

                for (; iter != end; iter++)
                    trace.emplace_back(iter->str());
            }
        }
    }

    std::ofstream file(std::format("{}/{}.{}.profiler.json", path, file_name, px::FormatTime("__{0:%g}_{0:%h}_{0:%d}_{0:%H}_{0:%OM}_{0:%OS}")));
    file.width(4);
    file << data;
}

PX_NAMESPACE_END();
