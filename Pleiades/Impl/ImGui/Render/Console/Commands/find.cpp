
#include <regex>
#include <set>
#include "../Console.hpp"

SG_NAMESPACE_BEGIN;

SG_COMMAND(
	find,
R"(
	Clear console output.
	USAGE:
		] find [flags] [files, ...]
	
	-n <count=0>:	Number of entries to print out.
	-r:				Use regular expression.
	-c:				Search for commands.
	-v:				Search for convars.
	-look:
		* name			: Lookup in command's name.
		* description	: Lookup in command's description.
		* plugin		: Lookup in command plugin's name.
)"
)
{
	size_t count = args.get_arg("n", 0);
	auto vals = args.get_val<std::vector<std::string>>();

	auto cmds = SG::console_manager.FindCommands("");
	std::set<ConCommand*> found_cmds;

	bool allow_cvars = args.contains("c");
	bool allow_cmds = args.contains("v");

	if (args.contains("r"))
	{
		try
		{
			std::vector<std::regex> regs;
			regs.reserve(vals.size());
			for (auto& val : vals)
				regs.emplace_back(val.c_str(), val.size());

			for (auto cmd : cmds)
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
				for (auto& reg : regs)
				{
					if (std::regex_search(cmd->name().begin(), cmd->name().end(), reg))
					{
						if (!found_cmds.insert(cmd).second)
							break;
					}
				}
			}
		}
		catch (const std::exception& ex)
		{
			SG::console_manager.Print(
				{ 255, 120, 120, 255 },
				std::format("Exception reported while matching for regular expression : \n{}", ex.what())
			);
			return nullptr;
		}
	}
	else
	{
		for (auto cmd : cmds)
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
			for (auto& val : vals)
			{
				if (cmd->name().contains(val))
				{
					if (!found_cmds.insert(cmd).second)
						break;
				}
			}
		}
	}

	size_t cmd_idx = 0;
	for (auto& cmd : found_cmds)
	{
		SG::console_manager.Print(
			{ 255, 255, 255, 255 },
			std::format("[{}] : {}", ++cmd_idx, cmd->name())
		);
	}

	return nullptr;
}

SG_NAMESPACE_END;