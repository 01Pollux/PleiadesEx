#include <iostream>
#include <format>
#include <fstream>
#include <list>

#include <px/string.hpp>

#include "Context.hpp"
#include "library/Manager.hpp"
#include "library/Module.hpp"
//#include "Impl/Console/config.hpp"

constexpr const char* GlobalInitFunction = "Tella_GetPlugin";
namespace fs = std::filesystem;

PluginContext::PluginContext(fs::path& root_path, const std::string& cur_name, const std::string& proc_name, std::string& out_err)
{
	root_path.append(cur_name).append(cur_name);

	bool start_paused = false;
	bool manual_mapped = false;
	// Check dependencies, and if the current process is supported in '*.config.json' first
	{
		std::ifstream file(root_path.string() + ".config.json");
		if (!file)
		{
			out_err = "Config missing.";
			return;
		}

		try
		{
			auto cfg{ nlohmann::json::parse(file, nullptr, true, true) };
			const auto info_sec = cfg.find("info");
			
			if (info_sec != cfg.end())
			{
				// check for the supported processes
				{
					if (const nlohmann::json& proc_list = (*info_sec)["process"];
						proc_list.is_array() && proc_list.size() > 0)
					{
						bool supported = false;
						for (const nlohmann::json& proc : proc_list)
						{
							if (proc == proc_name)
							{
								supported = true;
								break;
							}
						}
						if (!supported)
						{
							std::format_to(std::back_inserter(out_err), "Process \"{}\" is not supported.", proc_name);
							return;
						}
					}
				}

				// check for any dependencies
				{
					const auto& deps_list = (*info_sec)["deps"];
					if (!deps_list.empty())
					{
						bool found = false;
						for (auto& dir : fs::directory_iterator(LibraryManager::PluginsDir))
						{
							for (auto& deps : deps_list)
							{
								if (dir.path().stem() == deps.get_ref<const std::string&>())
								{
									found = true;
									break;
								}
							}
						}
						if (!found)
						{
							out_err = "Plugin missing some dependencies.";
							return;
						}
					}
				}

				bool auto_save{ };
				bool auto_load{ };
				// get the other vars
				for (auto var_and_name : {
						std::pair(&start_paused,	"paused"),
						std::pair(&manual_mapped,	"manual mapped"),
						std::pair(&auto_load,		"auto load"),
						std::pair(&auto_save,		"auto save")
					 })
				{
					const auto iter = info_sec->find(var_and_name.second);
					if (iter != info_sec->end() && iter->is_boolean())
					{
						*var_and_name.first = *iter;
					}
				}

				m_ShouldLoad = auto_load;
				m_ShouldSave = auto_save;
			}
		}
		catch (const std::exception& ex)
		{
			std::format_to(std::back_inserter(out_err), "Exception reported: \"{}\".", ex.what());
		}
	}

	{
		root_path += ".dll";

		if (!fs::exists(root_path))
		{
			out_err = "Failed to find dll";
			return;
		}
	}

	static_assert(std::is_same_v<decltype(root_path.c_str()), const wchar_t*>);
	std::unique_ptr<LibraryImpl> hMod{ static_cast<LibraryImpl*>(px::lib_manager.OpenLibrary(root_path.wstring(), false)) };

	if (!hMod)
	{
		std::format_to(std::back_inserter(out_err), "Failed to load dll ({}).", px::lib_manager.GetLastError());
		return;
	}

	PluginInitFn pFunc = static_cast<PluginInitFn>(hMod->FindByName(GlobalInitFunction).get());
	if (!pFunc)
	{
		std::format_to(std::back_inserter(out_err), "Failed to find init function (Code: {}).", px::lib_manager.GetLastError());
		return;
	}

	px::IPlugin* pl = pFunc();
	if (!pl)
	{
		out_err = "Failed to find get plugin from init function.";
		return;
	}

	m_Plugin = pl;
	m_PluginModule = std::move(hMod);

	pl->m_IsPaused = start_paused;
	pl->m_FileName = cur_name;
}

std::filesystem::path PluginContext::AutoExecConfigPath() const noexcept
{
	return std::format("{}/{}.autoexec", LibraryManager::ConfigDir, this->GetFileName());
}

void PluginContext::LoadConfig()
{
	/*std::ifstream file(AutoExecConfigPath());
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
	}*/
}

void PluginContext::SaveConfig()
{
	/*std::vector<px::IPlugin::FileConfigs> configs;
	m_Plugin->OnSaveConfig(configs);
	if (!configs.empty())
	{
		std::ofstream file(AutoExecConfigPath());
		for (auto& cfg : configs)
		{
			if (cfg.FileName.empty())
			{
				for (auto& cmd : cfg.Commands)
					file << cmd.first << ' ' << cmd.second << '\n';
				file << '\n';
			}
			else
			{
				std::ofstream other_cfg{ std::format("{}/{}.cfg", LibraryManager::ConfigDir, cfg.FileName) };
				for (auto& cmd : cfg.Commands)
					other_cfg << cmd.first << ' ' << cmd.second << '\n';
				file << "exec " << cfg.FileName << "\n\n";
			}
		}
	}*/
}

PluginContext::~PluginContext()
{
	if (m_Plugin)
		m_Plugin->OnPluginUnload();
}
