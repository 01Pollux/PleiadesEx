
#include <regex>
#include <set>
#include "../Console.hpp"

SG_NAMESPACE_BEGIN;

struct Boolcx
{
public:
	bool v{ };
	float X{ };

	static auto from_string(const std::string_view& value)
	{
		auto vals = CommandParser<std::string_view>::split(value);
		Boolcx bc;

		if (vals.size())
		{
			bc.v = CommandParser<bool>::from_string(vals[0]);
			bc.X = CommandParser<float>::from_string(vals[1]);
		}
		return bc;
	}

	std::string to_string() const
	{
		return std::format("[{}, {}]", v, X);
	}
};

ConVar<Boolcx> zzzzzzzz{ "my_var_something", Boolcx{ true, 2.0 }, "des" };

SG_COMMAND(
	find,
R"(
	Clear console output.
	USAGE:
		] find [flags] <files, ...>
	
	-c <count=0>:	Number of entries to print out.
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
	size_t count = args.get_arg("c", 0);
	auto vals = CommandParser<std::string_view>::split(args.get_val("<,>"));

	auto cmds = SG::console_manager.FindCommands("");
	std::set<ConCommand*> found_cmds;

	if (args.contains("r"))
	{
		try
		{
			std::vector<std::regex> regs;
			regs.reserve(vals.size());
			for (auto& val : vals)
				regs.emplace_back(val.data(), val.size());

			for (auto cmd : cmds)
			{
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