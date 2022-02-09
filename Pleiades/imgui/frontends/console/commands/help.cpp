
#include "plugins/Manager.hpp"
#include "../Console.hpp"
#include "console/Manager.hpp"

PX_COMMAND(
	help,
R"(For more information about a command, type "help <command>".
	USAGE:
		] help [command]...)"
)
{
	if (exec_info.value.empty())
	{
		px::console_manager.Print(
			std::string{ help_cmd.help() }
		);
	}
	else
	{
		const float word_size = ImGui::CalcTextSize(" ").y;

		for (auto& name : exec_info.value.split<std::string>(" "))
		{
			px::con_command* cmd = px::console_manager.FindCommand(name);
			if (cmd)
			{
				px::console_manager.Print(
					std::format("\nCommand: \"{}\"\nDescription:\n{}\n\n",cmd->name(), cmd->help())
				);
			}
			
		}
	}
}
