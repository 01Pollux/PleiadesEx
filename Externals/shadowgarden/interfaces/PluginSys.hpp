#pragma once

#include <map>
#include <vector>
#include <nlohmann/Json_Fwd.hpp>
#include "User/Version.hpp"
#include "InterfacesSys.hpp"

SG_NAMESPACE_BEGIN;

struct PluginInfo
{
	const char* m_Name;
	const char* m_Author;
	const char* m_Description;
	const char* m_Date;
	Version		m_Version;
};

class IPlugin
{
	friend class PluginContext;
public:
	IPlugin(const PluginInfo& info) noexcept : m_PluginInfo(info) { };

	/// <summary>
	/// Called when plugin is first loaded, use this callback to register interfaces only
	/// </summary>
	/// <returns>
	/// true if plugin is all set to be loaded, false otherwise
	/// </returns>
	virtual void OnPluginPreLoad(IPluginManager* ifacemgr) { }

	/// <summary>
	/// Called when plugin is first loaded
	/// </summary>
	/// <returns>
	/// true if plugin is all set to be loaded, false otherwise
	/// </returns>
	virtual bool OnPluginLoad(IPluginManager* ifacemgr) { return false; }

	/// <summary>
	/// Called when plugin is unloading
	/// </summary>
	virtual void OnPluginUnload() { }

	/// <summary>
	/// Called when all plugins is unloading
	/// </summary>
	virtual void OnAllPluginsLoaded() { }

	/// <summary>
	/// Called when all plugin is changing state
	/// </summary>
	/// <param name="pausing">true if plugin is paused, false otherwise</param>
	virtual void OnPluginPauseChange(bool pausing) { }

	/// <summary>
	/// Called when a plugin wants to save current data to json file
	/// </summary>
	virtual void OnSaveConfig(Json& cfg) { }

	/// <summary>
	/// Called when a plugin wants to load from json file to data
	/// </summary>
	virtual void OnReloadConfig(const Json& cfg) { }

	/// <summary>
	/// Get registered plugin info
	/// </summary>
	const PluginInfo* GetPluginInfo() const noexcept
	{
		return &m_PluginInfo;
	}

	/// <summary>
	/// Get plugin state
	/// </summary>
	bool IsPluginPaused() const noexcept
	{
		return m_IsPaused;
	}

	/// <summary>
	/// Set plugin state
	/// </summary>
	void SetPluginState(bool pause) noexcept
	{
		if (pause)
		{
			if (!m_IsPaused)
				this->OnPluginPauseChange(true);
		}
		else if (m_IsPaused)
			this->OnPluginPauseChange(false);
		
		m_IsPaused = pause;
	}

	/// <summary>
	/// Get plugin's file name
	/// </summary>
	const std::string& GetFileName() const noexcept
	{
		return m_FileName;
	}

private:
	std::string m_FileName;
	bool m_IsPaused;
	const PluginInfo m_PluginInfo;
};

SG_NAMESPACE_END;