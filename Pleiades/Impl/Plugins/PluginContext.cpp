#include <iostream>
#include <format>
#include <fstream>
#include <list>

#include "PluginContext.hpp"
#include "Impl/Library/LibrarySys.hpp"
#include <User/String.hpp>

SG_NAMESPACE_BEGIN;

namespace fs = std::filesystem;

PluginContext::PluginContext(fs::path& root_path, const std::string& cur_name, const std::string& proc_name, std::string& out_err)
{
	root_path.append(cur_name).append(cur_name);

	bool start_paused = false;
	bool is_x64 = false;
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
			Json cfg = Json::parse(file, nullptr, true, true);
			const auto info_sec = cfg.find(InfoSection::Name);
			
			if (info_sec != cfg.end())
			{
				// check for the supported processes
				{
					if (const Json& proc_list = (*info_sec)[InfoSection::SupportedProcesses];
						proc_list.is_array() && proc_list.size() > 0)
					{
						bool supported = false;
						for (const Json& proc : proc_list)
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
					const auto& deps_list = (*info_sec)[InfoSection::Dependencies];
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

				// get the other vars
				for (auto var_and_name : {
						std::pair(&start_paused,				InfoSection::Paused),
						std::pair(&m_InfoSection.m_ShouldLoad,	InfoSection::AutoLoad),
						std::pair(&m_InfoSection.m_ShouldSave,	InfoSection::AutoSave)
					 })
				{
					const auto iter = info_sec->find(var_and_name.second);
					if (iter != info_sec->end() && iter->is_boolean())
					{
						*var_and_name.first = *iter;
					}
				}

				// check if the plugin should be loaded in 64 bits
				const auto jis_x64 = info_sec->find("64 bits");
				is_x64 = jis_x64 != info_sec->end() && jis_x64->is_boolean() ? jis_x64->get<bool>() : false;
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

	std::unique_ptr<ILibrary> hMod;
	static_assert(std::is_same_v<decltype(root_path.c_str()), const wchar_t*>);

	if (is_x64)
		hMod.reset(lib_manager.OpenLibrary(root_path.c_str()));
	else
		hMod.reset(lib_manager.OpenLibrary(root_path.string().c_str()));

	if (!hMod)
	{
		std::format_to(std::back_inserter(out_err), "Failed to load dll ({}).", lib_manager.GetLastError());
		return;
	}

	PluginInitFn pFunc = static_cast<PluginInitFn>(hMod->FindByName(GlobalInitFunction).get());
	if (!pFunc)
	{
		std::format_to(std::back_inserter(out_err), "Failed to find init function (Code: {}).", lib_manager.GetLastError());
		return;
	}

	IPlugin* pl = pFunc();
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

std::filesystem::path PluginContext::GetConfigPath()
{
	const std::string& name = this->GetFileName();
	return std::format("{0}/{1}/{1}.config.json", LibraryManager::PluginsDir, name);
}

Json PluginContext::OpenConfig()
{
	std::ifstream file(GetConfigPath());
	return Json::parse(file, nullptr, true, true);
}

void PluginContext::LoadConfig()
{
	Json cfg = OpenConfig();
	std::ifstream file(GetConfigPath());
	file >> cfg;
	m_Plugin->OnReloadConfig(cfg[PluginContext::VarsSection]);
}

void PluginContext::SaveConfig()
{
	Json cfg = OpenConfig();
	m_Plugin->OnSaveConfig(cfg[PluginContext::VarsSection]);
	std::ofstream file(GetConfigPath());
	file.width(2);
	file << cfg;
}

PluginContext::~PluginContext()
{
	if (m_Plugin)
		m_Plugin->OnPluginUnload();
}


SG_NAMESPACE_END;