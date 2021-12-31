#include "Impl/Plugins/PluginManager.hpp"
#include "Impl/ImGui/Render/Console/Console.hpp"
#include "config.hpp"

SG_NAMESPACE_BEGIN;

ConsoleManager console_manager;

bool ConsoleManager::AddCommands(ConCommand* command)
{
	IPlugin* plugin = command->plugin();
	if (plugin && !SG::plugin_manager.FindContext(plugin))
		return false;

	ConCommand* cmd = command;
	while (cmd)
	{
		cmd->m_Plugin = plugin;
		imgui_console.m_Commands.push_back(cmd);
		cmd = std::exchange(cmd->m_NextCommand, nullptr);
	}

	return true;
}

bool ConsoleManager::RemoveCommand(ConCommand* command)
{
	PluginContext* pCtx = SG::plugin_manager.FindContext(command->plugin());
	if (!pCtx)
		return false;

	return std::erase(
		imgui_console.m_Commands,
		command
	) != 0;
}

void ConsoleManager::RemoveCommands()
{
	std::erase_if(
		imgui_console.m_Commands,
		[](ConCommand* cmd)
		{
			return cmd->plugin() != nullptr;
		}
	);
}

bool ConsoleManager::RemoveCommands(IPlugin* plugin)
{
	PluginContext* pCtx = SG::plugin_manager.FindContext(plugin);
	if (!pCtx)
		return false;

	return std::erase_if(
		imgui_console.m_Commands,
		[plugin](ConCommand* cmd)
		{
			return cmd->plugin() == plugin;
		}
	) != 0;
}

ConCommand* ConsoleManager::FindCommand(const std::string_view& name)
{
	for (ConCommand* cmd : imgui_console.m_Commands)
	{
		if (name == cmd->name())
			return cmd;
	}
	return nullptr;
}

std::vector<ConCommand*> ConsoleManager::FindCommands(IPlugin* plugin)
{
	std::vector<ConCommand*> cmds;
	for (ConCommand* cmd : imgui_console.m_Commands)
	{
		if (plugin == cmd->plugin())
			cmds.push_back(cmd);
	}
	return cmds;
}

std::vector<ConCommand*> ConsoleManager::FindCommands(const std::string_view& name)
{
	std::vector<ConCommand*> cmds;
	if (!name.size())
		cmds = imgui_console.m_Commands;
	else
	{
		for (ConCommand* cmd : imgui_console.m_Commands)
		{
			if (name == cmd->name())
				cmds.push_back(cmd);
		}
	}
	return cmds;
}

void ConsoleManager::Execute(const std::string_view& cmds)
{
    auto commands = [&str = cmds]()
    {
        auto iter = str.begin(), end = str.end(), last_begin = iter;
        std::vector<std::string_view> strs;

        while (iter != end)
        {
            while (iter != end && (*iter == ' ' || *iter == ';'))
                ++last_begin, ++iter;

            if (iter == end)
                break;

            enum class ParseState : char { None, SkipNext, InQuote, EndOfCmd } state = ParseState::None;

            while (iter != end)
            {
                if (state == ParseState::SkipNext)
                {
                    state = ParseState::None;
                    ++iter;
                    continue;
                }

                switch (*iter)
                {
                case '\\':
                {
                    state = ParseState::SkipNext;
                    break;
                }
                case '"':
                {
                    state = state == ParseState::InQuote ? ParseState::None : ParseState::InQuote;
                    break;
                }
                case ';':
                {
                    if (state != ParseState::InQuote)
                        state = ParseState::EndOfCmd;
                    break;
                }
                }

                if (++iter == end || state == ParseState::EndOfCmd)
                {
                    auto last_end = iter - 1;
                    while (last_end != last_begin && *last_end == ';')
                        --last_end;
                    strs.emplace_back(last_begin, last_end + 1);

                    if (iter != end)
                        last_begin = iter;
                    break;
                }
            }
        }

        return strs;
    }();

    for (const auto& cmd : commands)
    {
        auto [pCmd, cmd_name, cmd_val, cmd_args] =
            [cmd]()
        {
            std::string_view cmd_val;
            std::vector<std::pair<std::string_view, std::string_view>> args;

            auto iter = cmd.begin(), end = cmd.cend(), last_begin = iter;

            // seek first white and set [begin, cur( as cmd_name
            while (iter != end && *iter != ' ')
                ++iter;

            std::string_view cmd_name = { last_begin, iter };
            ConCommand* pCmd = SG::console_manager.FindCommand(cmd_name);

            // there is more than command name, fetch them
            if (pCmd && iter != end)
            {
                auto advance_arg = [end](std::string_view::const_iterator& last_begin) -> std::string_view::const_iterator
                {
                    while (last_begin != end && *last_begin == ' ')
                        ++last_begin;

                    auto iter = last_begin;
                    enum class ParseState : char { None, SkipNext, InQuote, EndOfCmd } state = ParseState::None;

                    while (iter != end)
                    {
                        if (state == ParseState::SkipNext)
                        {
                            state = ParseState::None;
                            ++iter;
                            continue;
                        }

                        switch (*iter)
                        {
                        case '\\':
                        {
                            state = ParseState::SkipNext;
                            break;
                        }
                        case '"':
                        {
                            state = state == ParseState::InQuote ? ParseState::None : ParseState::InQuote;
                            break;
                        }
                        case ' ':
                        {
                            if (state != ParseState::InQuote)
                                state = ParseState::EndOfCmd;
                            break;
                        }
                        }

                        if (state == ParseState::EndOfCmd)
                        {
                            return iter;
                        }
                        else ++iter;
                    }

                    return end;
                };

                while (iter != end)
                {
                    while (iter != end && *iter == ' ')
                        ++iter;

                    if (iter == end)
                        break;

                    // seek arg, starts with '-'
                    if (*iter == '-')
                    {
                        last_begin = iter + 1;
                        while (iter != end && (*iter != ' ' && *iter != ':'))
                            ++iter;

                        std::string_view  arg_name{ last_begin, iter++ };
                        last_begin = iter;

                        enum class ParseState : char { None, SkipNext, InQuote, EndOfCmd } state = ParseState::None;
                        while (iter != end)
                        {
                            if (state == ParseState::SkipNext)
                            {
                                state = ParseState::None;
                                ++iter;
                                continue;
                            }

                            switch (*iter)
                            {
                            case '\\':
                            {
                                state = ParseState::SkipNext;
                                break;
                            }
                            case '"':
                            {
                                state = state == ParseState::InQuote ? ParseState::None : ParseState::InQuote;
                                break;
                            }
                            case ' ':
                            case '-':
                            {
                                if (state != ParseState::InQuote)
                                    state = ParseState::EndOfCmd;
                                break;
                            }
                            }

                            if (state == ParseState::EndOfCmd)
                                break;
                            else ++iter;
                        }

                        if (last_begin != end && *last_begin == '\"')
                            ++last_begin;
                        auto tmp_iter = iter - 1;
                        if (tmp_iter > last_begin && *tmp_iter == '\"')
                            --tmp_iter;

                        std::string_view  arg_val{ last_begin, tmp_iter };
                        args.emplace_back(
                            arg_name,
                            arg_val
                        );

                        if (iter == end)
                            break;
                    }
                    else if (*iter != ' ')
                    {
                        cmd_val = { iter, end };
                        break;
                    }
                    ++iter;
                }
            }

            return std::tuple{ pCmd, cmd_name, cmd_val, args };
        }();

        if (!pCmd)
        {
            constexpr uint32_t red_clr = 255 | 120 << 0x8 | 120 << 0x10 | 255 << 0x18;
            this->Print(
                red_clr,
                std::format("Command '{}' is not a command nor a convar.", cmd_name)
            );
        }
        else
        {
            const char* callback_str = pCmd->exec_callback()(
                pCmd,
                { std::move(cmd_args), cmd_val }
            );
            if (callback_str)
            {
                constexpr uint32_t red_clr = 255 | 120 << 0x8 | 120 << 0x10 | 255 << 0x18;
                this->Print(
                    red_clr,
                    callback_str
                );
            }
        }
    }
}

void ConsoleManager::Clear(size_t size, bool is_history)
{
	if (is_history)
	{
		if (!size || imgui_console.m_HistoryCmds.size() <= size)
			imgui_console.m_HistoryCmds.clear();
		else
		{
			auto end = imgui_console.m_HistoryCmds.end();
			auto iter = end - size;
			imgui_console.m_HistoryCmds.erase(iter, end);
		}
	}
	else
	{
		if (!size || imgui_console.m_Logs.size() <= size)
			imgui_console.m_Logs.clear();
		else
		{
			auto end = imgui_console.m_Logs.end();
			auto iter = end - size;
			imgui_console.m_Logs.erase(iter, end);
		}
	}
}

void ConsoleManager::Print(uint32_t color, const std::string& msg)
{
	imgui_console.m_Logs.emplace_back(
        color,
		msg
	);
}

SG_NAMESPACE_END;