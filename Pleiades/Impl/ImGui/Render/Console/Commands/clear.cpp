#include "../Console.hpp"

PX_NAMESPACE_BEGIN();

PX_COMMAND(
	clear,
R"(
	Clear console output.
	USAGE:
		] clear [flags]

	-c <count=0>:	Number of entries to erase from console.
	-h:				Clear history instead.
)"
)
{
	size_t count = args.get_arg("c", 0);
	bool history_only = args.contains("h");
	px::console_manager.Clear(count, history_only);

	return nullptr;
}

PX_NAMESPACE_END();