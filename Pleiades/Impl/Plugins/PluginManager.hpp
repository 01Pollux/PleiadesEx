#pragma once

#include <map>
#include "Impl/Plugins/PluginContext.hpp"

SG_NAMESPACE_BEGIN;

struct InterfaceInfo
{
	IInterface* IFace;
	IPlugin* Plugin;
};

class DLLManager : public IPluginManager
{
public:
	bool BasicInit();

	void LoadAllDLLs(const Json& plugins);

	void UnloadAllDLLs();

private:
	std::string GetProcessName();

	void LoadOneDLL(const std::string& proc_name, std::filesystem::path& path, const std::string& name);

	PluginContext* FindContext(const IPlugin* plugin) noexcept
	{
		for (auto& ctx : m_Plugins)
		{
			if (ctx->GetPlugin() == plugin)
				return ctx.get();
		}
		return nullptr;
	}

	auto FindContext(const std::string& name) noexcept
	{
		auto iter = m_Plugins.begin();
		for (const auto end = m_Plugins.end(); iter != end; iter++)
		{
			if (iter->get()->GetFileName() == name)
				break;
		}
		return iter;
	}

public:
	// Inherited via IInterfaceManager
	bool ExposeInterface(const char* name, IInterface* iface, IPlugin* owner) override;

	bool RequestInterface(const std::string& iface_name, IInterface** iface) override;

	IPlugin* FindPlugin(const std::string& name) override;

	bool BindPlugin(IPlugin* caller, const char* name, bool is_required) override;

	void RequestShutdown(IPlugin* iface) override;

	void Shutdown() noexcept override;

	void UpdatePluginConfig(IPlugin* plugin, PlCfgLoadType loadtype) override;

	Version GetHostVersion() override;

	IPlugin* LoadPlugin(const std::string& name) override;

	bool UnloadPlugin(const std::string& name) override;

	void UnloadPlugin_Internal(PluginContext* context);

private:
	std::list<std::unique_ptr<PluginContext>>	m_Plugins;
	std::map<std::string, InterfaceInfo>		m_Interfaces;

	bool m_PrintedFirstTime{ };
};

extern DLLManager plugin_manager;

SG_NAMESPACE_END;