#include "../Console.hpp"

SG_NAMESPACE_BEGIN;

SG_COMMAND(
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
	SG::console_manager.Clear(count, history_only);

	return nullptr;
}

SG_NAMESPACE_END;