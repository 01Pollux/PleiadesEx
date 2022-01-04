#include "Logger.hpp"
#include <imgui/imgui_internal.h>

PX_NAMESPACE_BEGIN();

// TODO: logs -> rename Message to Warning
// TODO: for Fatal : bright red, Error Red brown, Warning : yellow, Debug Dark green
// 
// if value is cutsom, TODO investigate how json write to stream
// REPORT TO MSDN about Intellisense not showing suggestion before first time modules
// RESOLVE Info section with only one entry -> Message: ... 

bool ImGuiPlLogSection::FilterPlugin(iterator_type iter) const
{
    if (Filter.Filters.empty())
        return true;

    for (auto& txt : Filter.Filters)
    {
        if (txt.empty())
            continue;

        const std::string_view str(txt.b, txt.e);
        const bool negate = str[0] == '-';
        const size_t offset = negate ? 1 : 0;

        // if we didn't finish the command yet
        if (str.size() <= offset + 3)
            continue;
        // check if it's a valid command for us to use
        if (str[offset] != '/' || str[offset + 2] != '=')
            continue;

        if (str[offset + 1] == 'p')
        {
            const std::string_view final_str(str.begin() + offset + 3, str.end());
            const std::string_view iter_key = iter->FileName;

            if (ImStristr(iter_key.data(), iter_key.data() + iter_key.size(), final_str.data(), final_str.data() + final_str.size()) != NULL)
                return negate ? false : true;
        }
        else // ignore other commands
            return true;
    }

    // Implicit * grep
    return Filter.CountGrep == 0;
}

bool ImGuiJsLogInfo::FilterInfo(const ImGuiTextFilter& filter, nlohmann::json::const_iterator iter, bool is_type) const
{
    if (filter.Filters.empty())
        return true;

    for (auto& txt : filter.Filters)
    {
        if (txt.empty())
            continue;

        const std::string_view str(txt.b, txt.e);
        const bool negate = str[0] == '-';
        const size_t cmd_offset = negate ? 1 : 0;

        // if we didn't finish the command yet
        if (str.size() <= cmd_offset + 3)
            continue;
        // check if it's a valid command for us to use
        if (str[cmd_offset] != '/' || str[cmd_offset + 2] != '=')
            continue;

        const std::string_view final_str(str.begin() + cmd_offset + 3, str.end());
        const std::string_view iter_key = iter.key();

        if (is_type)
        {
            // /t=type
            if (str[cmd_offset + 1] == 't')
            {
                for (std::string_view log_type : {
                    "debug",
                        "message",
                        "error",
                        "fatal"
                })
                {
                    if (ImStristr(iter_key.data(), iter_key.data() + iter_key.size(), log_type.data(), log_type.data() + log_type.size()))
                    {
                        if (ImStristr(final_str.data(), final_str.data() + final_str.size(), log_type.data(), log_type.data() + log_type.size()) != NULL)
                            return negate ? false : true;
                        break;
                    }
                }
                return false;
            }
            else return true;
        }
        switch (str[cmd_offset + 1])
        {
        case 'd': // /d=date
        {
            enum class CmpType
            {
                Less,
                LessEq,
                Eq,
                Greater,
                GreaterEq
            };

            CmpType cmp_type = CmpType::Eq;
            size_t begin_offset = 0;

            switch (final_str[0])
            {
            case '\0':
                return true;
            case '<':
            {
                if (final_str[1] == '=')
                {
                    cmp_type = CmpType::LessEq;
                    begin_offset = 2;
                }
                else
                {
                    cmp_type = CmpType::Less;
                    begin_offset = 1;
                }
                break;
            }
            case '>':
            {
                if (final_str[1] == '=')
                {
                    cmp_type = CmpType::GreaterEq;
                    begin_offset = 2;
                }
                else
                {
                    cmp_type = CmpType::Greater;
                    begin_offset = 1;
                }
                break;
            }
            }

            size_t size = (final_str.size() - begin_offset) <= iter_key.size() ? (final_str.size() - begin_offset) : iter_key.size();
            for (size_t i = 0; i < size; i++)
            {
                const char a = final_str[i + begin_offset], b = iter_key[i];
                switch (cmp_type)
                {
                case CmpType::Less:
                {
                    if (!(a > b))
                        return false;
                    break;
                }
                case CmpType::LessEq:
                {
                    if (!(a >= b))
                        return false;
                    break;
                }
                [[likely]]
                case CmpType::Eq:
                {
                    if (a != b)
                        return false;
                    break;
                }
                case CmpType::Greater:
                {
                    if (!(a < b))
                        return false;
                    break;
                }
                case CmpType::GreaterEq:
                {
                    if (!(a <= b))
                        return false;
                    break;
                }
                }
            }

            return true;
        }

        case 'm': // [-]/m=message
        case 'e': // [-]/e=exception
        case 'k': // [-]/k=key
        {
            auto info = iter->find("Info");
            if (info != iter->end())
            {
                const char tag = str[cmd_offset + 1];

                if (tag != 'k')
                {
                    auto msg_iter = info->find(tag == 'm' ? "Message" : "Exception");
                    if (msg_iter != info->end() && msg_iter->is_string())
                    {
                        const std::string_view msg = msg_iter->get_ref<const std::string&>();
                        if (ImStristr(msg.data(), msg.data() + msg.size(), final_str.data(), final_str.data() + final_str.size()) != NULL)
                            return negate ? false : true;
                    }
                }
                else
                {
                    auto key_iter = info->find(final_str);
                    return  negate ? key_iter == info->end() : key_iter != info->end();
                }
            }

            return !negate;
        }
        }
    }

    // Implicit * grep
	return filter.CountGrep == 0;
}

PX_NAMESPACE_END();