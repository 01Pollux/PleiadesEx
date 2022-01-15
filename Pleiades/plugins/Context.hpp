#pragma once

#include <filesystem>

#include <nlohmann/Json.hpp>
#include <px/interfaces/PluginSys.hpp>
#include <px/Interfaces/LibrarySys.hpp>

#include <px/version.hpp>
#include <px/intptr.hpp>

using PluginInitFn = px::IPlugin* (__cdecl*)();

class PluginContext
{
public:
	PluginContext(std::filesystem::path& root_path, const std::string& cur_name, const std::string& proc_name, std::string& out_error);
	~PluginContext();

	[[nodiscard]] px::IPlugin* GetPlugin() noexcept
	{
		return m_Plugin;
	}

	[[nodiscard]] const px::PluginInfo* GetPluginInfo() const noexcept
	{
		return m_Plugin->GetPluginInfo();
	}

	void OnPluginFullLoaded()
	{
		if (m_WasLoaded)
		{
			m_WasLoaded = true;
			m_Plugin->OnAllPluginsLoaded();
		}
	}

public:
	void LoadConfig();
	void SaveConfig();

	[[nodiscard]] const std::string& GetFileName() const noexcept { return m_Plugin->GetFileName(); }

	[[nodiscard]] bool ShouldAutoLoad() const noexcept { return m_ShouldLoad; }
	[[nodiscard]] bool ShouldAutoSave() const noexcept { return m_ShouldSave; }

	[[nodiscard]] bool HasDependencies() const noexcept { return !m_Dependencies.empty(); }
	[[nodiscard]] auto GetDependencies() const noexcept { return m_Dependencies; }
	[[nodiscard]] auto& GetDependencies() noexcept { return m_Dependencies; }

private:
	/// <summary>
	/// build's path to plugin's config
	/// Plugins/'m_FileName'/'m_FileName'.config.json
	/// </summary>
	std::filesystem::path AutoExecConfigPath() const noexcept;

	px::IPlugin*					m_Plugin{ };
	std::unique_ptr<px::ILibrary>	m_PluginModule;

	std::vector<px::IPlugin*>		m_Dependencies;

	bool m_ShouldLoad : 1{ };
	bool m_ShouldSave : 1{ };
	bool m_WasLoaded : 1{ };
};

/*
{
  "info": {
	"paused": true,		//	if true, plugin will start up paused
	"process": [		//	the plugin will only be loaded into those processes
	  "*.exe"
	],
	"deps": [			//	the plugin will requires this list of plugins in order to work propertly
	  "HelloWorldExtension"
	],
	"auto load": true,	//	start up by calling IPlugin::OnLoadConfig during load
	"auto save": true	//	end up calling IPlugin::OnSaveConfig during unload
  },
}
*/
