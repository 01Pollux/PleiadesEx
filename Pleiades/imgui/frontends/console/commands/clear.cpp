#include "../Console.hpp"
#include "console/Manager.hpp"

PX_COMMAND(
	clear,
R"(Clear console output.
USAGE:
	] clear [flags]...

FLAGS:
	-h, --help		  show help message.
	-c, --count		Number of entries to erase from console.
	--history		   Clear history instead.)",
	{
		px::cmd_mask{ "help", 'h', false, true },
		px::cmd_mask{ "count", 'c' },
		px::cmd_mask{ "history", 'h', false, true }
	}
)
{
	px::console_manager.Clear(exec_info.args.get<size_t>("count", 0), exec_info.args.contains("history"));
}
