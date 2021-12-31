
#include "../Console.hpp"
#include "Impl/Plugins/PluginManager.hpp"

SG_NAMESPACE_BEGIN;

SG_COMMAND(
	help,
R"(
	For more information about a command, type "help <command>".
	USAGE:
		] find [flags] [command]
	
	-ci:	Display list of commands registered in console internally.
	-ce:	Display list of commands registered in console externally.

	-vi:	Display list of convars registered in console internally.
	-ve:	Display list of convars registered in console externally.
)"
)
{
		if (args.arg_size())
		{
			auto cmds = SG::console_manager.FindCommands("");
			std::vector<SG::ConCommand*> final_cmds;

			{
				bool cmds_e_only = args.contains("ci");
				bool cmds_i_only = args.contains("ce");

				bool cvars_e_only = args.contains("vi");
				bool cvars_i_only = args.contains("ve");

				size_t cmd_idx = 0;
				for (auto& cmd : cmds)
				{
					if (cmd->is_command())
					{
						if (cmd->plugin())
						{
							if (!cmds_e_only)
								continue;
						}
						else if (!cmds_i_only)
							continue;
					}
					else
					{
						if (cmd->plugin())
						{
							if (!cvars_e_only)
								continue;
						}
						else if (!cvars_i_only)
							continue;
					}

					SG::console_manager.Print(
						{ 255, 120, 120, 255 },
						std::format("[{}] : {}", ++cmd_idx, cmd->name())
					);
				}
			}

			return nullptr;
		}
		else
		{
			ConCommand* cmd;
			if (args.val_size())
			{
				auto target_cmd = args.get_val<std::string>("");
				cmd = SG::console_manager.FindCommand(target_cmd);
				if (!cmd)
				{
					SG::console_manager.Print(
						{ 255, 120, 120, 255 },
						std::format("Command '{}' is not a command nor a convar", target_cmd)
					);
					return nullptr;
				}
			}
			else cmd = pCmd;

			{
				SG::console_manager.Print(
					{ 255, 255, 255, 255 },
					std::format(
R"(Name:	'{}'
Plugin:	'{}'
Description:
{})", 
					cmd->name(), 
					!cmd->plugin() ? "Internal" : cmd->plugin()->GetPluginInfo()->m_Name,
					cmd->description()
					)
				);
			}
			return nullptr;
		}
}

SG_NAMESPACE_END;