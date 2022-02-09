
#include <regex>
#include <set>
#include "../Console.hpp"
#include "console/Manager.hpp"

PX_COMMAND(
	find,
R"(Find command/convars currently registered.
USAGE:
	] find [flags] [files, ...]

FLAGS:
	-h, --help			   show help message.
	-n, --count			 Number of entries to print out.
	-r, --regex			  Use regular expression.
	-c, --commands	Search for commands.
	-v, --convars		 Search for convars.)",
	{
		px::cmd_mask{ "help",		'h', false, true },

		px::cmd_mask{ "count",		'n' },
		px::cmd_mask{ "regex",		'r', false, true },

		px::cmd_mask{ "commands",	'c', false, true },
		px::cmd_mask{ "convars",	'v', false, true }
	}
)
{

	size_t count = exec_info.args.get<size_t>("count", 0);
	auto vals = exec_info.value.split<std::string_view>();

	const auto& commands_info = px::console_manager.GetCommands();
	std::set<px::con_command*> found_cmds;

	bool allow_cmds = exec_info.args.contains("commands");
	bool allow_cvars = exec_info.args.contains("convars");

	if (exec_info.args.contains("regex"))
	{
		try
		{
			std::vector<std::regex> regs;
			regs.reserve(vals.size());
			for (const auto& val : vals)
				regs.emplace_back(std::string{ val.data(), val.data() + val.size() });

			for (auto& pl_and_cmds : commands_info)
			{
				for (auto cmd : pl_and_cmds.Commands)
				{
					if (cmd->is_command())
					{
						if (!allow_cmds)
							continue;
					}
					else
					{
						if (!allow_cvars)
							continue;
					}
					for (const auto& reg : regs)
					{
						if (std::regex_search(cmd->name().begin(), cmd->name().end(), reg))
						{
							if (!found_cmds.insert(cmd).second)
								break;
						}
					}
				}
			}
		}
		catch (const std::exception& ex)
		{
			px::console_manager.Print(
				{ 255, 120, 120, 255 },
				std::format("Exception reported while matching for regular expression : \n{}", ex.what())
			);
			return;
		}
	}
	else
	{
		for (auto& pl_and_cmds : commands_info)
		{
			for (auto cmd : pl_and_cmds.Commands)
			{
				if (cmd->is_command())
				{
					if (!allow_cmds)
						continue;
				}
				else
				{
					if (!allow_cvars)
						continue;
				}
				for (const auto& val : vals)
				{
					if (cmd->name().contains(val))
					{
						if (!found_cmds.insert(cmd).second)
							break;
					}
				}
			}
		}
	}

	size_t cmd_idx = 0;
	for (auto& cmd : found_cmds)
	{
		px::console_manager.Print(
			{ 255, 255, 255, 255 },
			std::format("[{}] : {}", ++cmd_idx, cmd->name())
		);
	}
}
