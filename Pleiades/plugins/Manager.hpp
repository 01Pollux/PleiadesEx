#pragma once

#include <map>
#include <px/interfaces/PluginSys.hpp>
#include "Context.hpp"

struct InterfaceInfo
{
	px::IInterface* IFace;
	px::IPlugin* Plugin;
};

class DLLManager : public px::IPluginManager
{
public:
	bool BasicInit();

	void BasicShutdown();

	void LoadAllDLLs(const nlohmann::json& plugins);

	void UnloadAllDLLs();

	[[nodiscard]] PluginContext* FindContext(const px::IPlugin* plugin) noexcept
	{
		for (auto& ctx : m_Plugins)
		{
			if (ctx->GetPlugin() == plugin)
				return ctx.get();
		}
		return nullptr;
	}

	[[nodiscard]] auto FindContext(const std::string& name) noexcept
	{
		auto iter = m_Plugins.begin();
		for (; iter != m_Plugins.end(); iter++)
		{
			if (iter->get()->GetFileName() == name)
				break;
		}
		return iter;
	}

private:
	[[nodiscard]] std::string GetProcessName();

	void LoadOneDLL(const std::string& proc_name, std::filesystem::path& path, const std::string& name);

public:
	// Inherited via IInterfaceManager
	bool ExposeInterface(const std::string& name, IInterface* iface, px::IPlugin* owner) override;

	bool RequestInterface(const std::string& iface_name, IInterface** iface) override;

	px::IPlugin* FindPlugin(const std::string& name) override;

	bool BindPlugin(px::IPlugin* caller, const char* name, bool is_required) override;

	void RequestShutdown(px::IPlugin* iface) override;

	void Shutdown() noexcept override;

	void UpdatePluginConfig(px::IPlugin* plugin, px::PlCfgLoadType loadtype) override;

	px::version GetHostVersion() override;

	px::IPlugin* LoadPlugin(const std::string& name) override;

	bool UnloadPlugin(const std::string& name) override;

	void UnloadPlugin_Internal(PluginContext* context);

private:
	std::list<std::unique_ptr<PluginContext>>	m_Plugins;
	std::map<std::string, InterfaceInfo>		m_Interfaces;

	bool m_PrintedFirstTime{ };
};

PX_NAMESPACE_BEGIN();
inline DLLManager plugin_manager;
PX_NAMESPACE_END();