#pragma once

#include <filesystem>

#include <nlohmann/Json.hpp>
#include <shadowgarden/interfaces/PluginSys.hpp>
#include <shadowgarden/Interfaces/LibrarySys.hpp>

#include <shadowgarden/users/Version.hpp>
#include <shadowgarden/users/IntPtr.hpp>


SG_NAMESPACE_BEGIN;

using PluginInitFn = SG::IPlugin* (__cdecl*)();
constexpr const char* GlobalInitFunction = "Tella_GetPlugin";

class ConCommand;

class PluginContext
{
public:
	class InfoSection
	{
		friend class PluginContext;
	public:
		static constexpr const char* Name = "info";

		static constexpr const char* Paused = "paused";

		static constexpr const char* SupportedProcesses = "process";
		static constexpr const char* Dependencies = "deps";

		static constexpr const char* AutoLoad = "auto load";
		static constexpr const char* AutoSave = "auto save";

	private:
		bool	m_IsInterface{ };
		bool	m_ShouldLoad{ };
		bool	m_ShouldSave{ };
	};
	static constexpr const char* VarsSection = "vars";

public:

	PluginContext(std::filesystem::path& root_path, const std::string& cur_name, const std::string& proc_name, std::string& out_error);
	~PluginContext();

	IPlugin* GetPlugin() noexcept
	{
		return m_Plugin;
	}

	const PluginInfo* GetPluginInfo() const noexcept
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

	const std::string& GetFileName() const noexcept { return m_Plugin->GetFileName(); }

	bool ShouldAutoLoad() const noexcept { return m_InfoSection.m_ShouldLoad; }
	bool ShouldAutoSave() const noexcept { return m_InfoSection.m_ShouldSave; }

	bool HasDependencies() const noexcept { return !m_Dependencies.empty(); }
	auto GetDependencies() const noexcept { return m_Dependencies; }
	auto GetDependencies() noexcept { return m_Dependencies; }

private:
	/// <summary>
	/// build's path to plugin's config
	/// Plugins/'m_FileName'/'m_FileName'.config.json
	/// </summary>
	std::filesystem::path AutoExecConfigPath() const noexcept;

	IPlugin*					m_Plugin{ };
	std::unique_ptr<ILibrary>	m_PluginModule;

	std::vector<IPlugin*>		m_Dependencies;

	InfoSection	m_InfoSection;
	bool m_WasLoaded{ };
};

/*
{
  "info": {
	"paused": true,		//	if true, plugin will start up paused
	"interface": false,	//	if true, plugin is sharing interface(s)
	"process": [		//	the plugin will only be loaded into those processes
	  "*.exe"
	],
	"deps": [			//	the plugin will requires this list of plugins in order to work propertly
	  "HelloWorldExtension"
	],
	"auto load": true,	//	start up by calling IPlugin::OnLoadConfig during load
	"auto save": true	//	end up calling IPlugin::OnSaveConfig during unload
  },
  "vars": {
	"szTextToDraw": "Hello World"
  }
}
*/

SG_NAMESPACE_END;