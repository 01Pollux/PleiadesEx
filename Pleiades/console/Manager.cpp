#include "plugins/Manager.hpp"
#include "Manager.hpp"


void ConsoleManager::AddCommands(
    px::IPlugin* plugin,
    px::con_command::entries_type::iterator begin,
    px::con_command::entries_type::iterator end
)
{
    auto iter = std::find_if(
        px::imgui_console.m_Commands.begin(),
        px::imgui_console.m_Commands.end(),
        [plugin](auto& wrap)
        {
            return wrap.Plugin == plugin;
        }
    );
    if (iter == px::imgui_console.m_Commands.end())
    {
        std::vector<px::con_command*> entries;
        entries.reserve(std::distance(begin, end));
        while (begin != end)
        {
            entries.emplace_back(begin->second);
            ++begin;
        }

        px::imgui_console.m_Commands.emplace_back(
            std::move(entries), plugin
        );
    }
    else
    {
        iter->Commands.reserve(iter->Commands.size() + std::distance(begin, end));
        while (begin != end)
        {
            iter->Commands.emplace_back(begin->second);
            ++begin;
        }
    }
}

bool ConsoleManager::RemoveCommand(px::con_command* command)
{
    for (auto iter = px::imgui_console.m_Commands.begin(); iter != px::imgui_console.m_Commands.end(); iter++)
    {
        bool do_break = false;
        for (auto cmd_iter = iter->Commands.begin(); cmd_iter != iter->Commands.end(); cmd_iter++)
        {
            if (*cmd_iter == command)
            {
                iter->Commands.erase(cmd_iter);
                break;
            }
        }
        if (do_break)
        {
            if (iter->Commands.empty())
                px::imgui_console.m_Commands.erase(iter);
            return true;
        }
    }
    return false;
}

bool ConsoleManager::RemoveCommands(px::IPlugin* plugin)
{
	PluginContext* pCtx = px::plugin_manager.FindContext(plugin);
	if (!pCtx)
		return false;

    for (auto iter = px::imgui_console.m_Commands.begin(); iter != px::imgui_console.m_Commands.end(); iter++)
    {
        if (iter->Plugin == plugin)
        {
            px::imgui_console.m_Commands.erase(iter);
            return true;
        }
    }
    return false;
}

const std::vector<ImGui_Console::CmdWrapper>& ConsoleManager::GetCommands() const noexcept
{
    return px::imgui_console.m_Commands;
}

px::con_command* ConsoleManager::FindCommand(std::string_view name)
{
	for (auto& iter : px::imgui_console.m_Commands)
	{
        for (auto cmd : iter.Commands)
        {
            if (!cmd->name().compare(name))
                return cmd;
        }
	}
	return nullptr;
}

std::vector<px::con_command*> ConsoleManager::FindCommands(px::IPlugin* plugin)
{
    for (auto& iter : px::imgui_console.m_Commands)
	{
        if (iter.Plugin == plugin)
            return iter.Commands;
	}
    return{ };
}

void ConsoleManager::Execute(std::string_view in_cmd)
{
    constexpr uint32_t red_clr = 255 | 120 << 0x8 | 120 << 0x10 | 255 << 0x18;
    try
	{
        std::vector<px::con_command*> cmds;
        auto args_infos = px::cmd_parser::parse(
            in_cmd,
            [this, &cmds](const std::string_view cmd_name, auto& begin, auto& end)
            {
                px::con_command* cmd = FindCommand(cmd_name);
                if (!cmd)
                {
                    this->Print(
                        red_clr,
                        std::format(R"(Command '{}' doesnt exists.)", cmd_name)
                    );
                    return false;
                }
                cmds.push_back(cmd);
                begin = cmd->masks().begin();
                end = cmd->masks().end();
                return true;
            }
        );
        
        px::con_command* help_cmd{ };
        for (size_t i = 0; i < cmds.size(); i++)
        {
            if (args_infos[i].args.contains("help"))
            {
                if (!help_cmd)
                    help_cmd = FindCommand("help");

                if (help_cmd)
                    (*help_cmd)(cmds[i]->name());
            }
            else
            {
                (*cmds[i])(std::move(args_infos[i]));
            }
        }
	}
    catch (const std::exception& ex)
    {
        this->Print(
            red_clr,
            std::format(
R"(Exception reported while parsing command.
Command: {}.
Exception: {}.)",
            in_cmd,
            ex.what()
            )
        );
    }
}

void ConsoleManager::Clear(size_t size, bool is_history)
{
	if (is_history)
	{
		if (!size || px::imgui_console.m_HistoryCmds.size() <= size)
            px::imgui_console.m_HistoryCmds.clear();
		else
		{
			auto end = px::imgui_console.m_HistoryCmds.end();
			auto iter = end - size;
            px::imgui_console.m_HistoryCmds.erase(iter, end);
		}
	}
	else
	{
		if (!size || px::imgui_console.m_Logs.size() <= size)
            px::imgui_console.m_Logs.clear();
		else
		{
			auto end = px::imgui_console.m_Logs.end();
			auto iter = end - size;
            px::imgui_console.m_Logs.erase(iter, end);
		}
	}
}

void ConsoleManager::Print(uint32_t color, const std::string& msg)
{
    px::imgui_console.m_Logs.emplace_back(
        color,
		msg
	);
}
