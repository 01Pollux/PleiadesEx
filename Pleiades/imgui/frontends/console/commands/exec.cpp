#include "../Console.hpp"
#include "library/Manager.hpp"

PX_COMMAND(
	exec,
R"(
	Execute a config file.
	USAGE:
		] exec <filename>.cfg
)"
)
{
	if (args.has_val())
	{
		for (auto& config_name : args.get_val<std::vector<std::string>>())
		{
			std::ifstream file(std::format("{}/{}.cfg", LibraryManager::ConfigDir, config_name));
			if (file)
			{
				std::string line, cmds;
				while (std::getline(file, line))
				{
					auto iter = line.begin(), end = line.end();
					bool is_comment = false;

					while (iter != end)
					{
						if (*iter == ' ')
						{
							++iter;
							continue;
						}
						else if (*iter == '#')
							is_comment = true;
						break;
					}

					if (is_comment)
						continue;

					cmds += std::move(line) + ';';
				}
				if (!cmds.empty())
					px::console_manager.Execute(cmds);
			}
			else
			{
				px::console_manager.Print(
					255 | 120 << 0x8 | 120 << 0x10 | 255 << 0x18,
					std::format("Config '{}' doesn't exists.", config_name)
				);
			}
		}
	}
	return nullptr;
}