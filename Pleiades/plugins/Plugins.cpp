#include <filesystem>

#include <px/modules.hpp>
#include <px/string.hpp>

#include "Manager.hpp"
#include "library/Manager.hpp"
#include "Logs/Logger.hpp"
//#include "/imgui_iface.hpp"
//#include "Console/config.hpp"


void DLLManager::LoadAllDLLs(const nlohmann::json& plugins)
{
	namespace fs = std::filesystem;

	{
		std::string procName = GetProcessName();

		for (const fs::path mainpath(LibraryManager::PluginsDir); const std::string& name : plugins)
		{
			fs::path path = mainpath;
			LoadOneDLL(procName, path, name);
		}
	}

	for (auto& ctx : m_Plugins)
		ctx->GetPlugin()->OnPluginPreLoad(this);

	std::erase_if(
		m_Plugins,
		[this] (auto& ctx)
	{
		px::IPlugin* pl = ctx->GetPlugin();
		if (!pl->OnPluginLoad(this))
		{
			PX_LOG_ERROR(
				PX_MESSAGE("Failed to load Plugin, check its log file."),
				PX_LOGARG("Name", ctx->GetFileName())
			);
			return true;
		}

		return false;
	}
	);

	{
		for (auto& ctx : m_Plugins)
		{
			px::IPlugin* pl = ctx->GetPlugin();
			try
			{
				if (ctx->ShouldAutoLoad())
					ctx->LoadConfig();
				ctx->OnPluginFullLoaded();
			}
			catch (const std::exception& ex)
			{
				PX_LOG_ERROR(
					PX_MESSAGE("Exception reported while reading plugin's config."),
					PX_LOGARG("Exception", ex.what())
				);
			}
		}
	}
}

px::IPlugin* DLLManager::LoadPlugin(const std::string& name)
{
	namespace fs = std::filesystem;
	px::IPlugin* pl{ };
	{
		if (FindContext(name) != m_Plugins.end())
			return pl;

		std::unique_ptr<PluginContext> ctx;
		// check if the path exits, then build plugin's context
		{
			const fs::path mainpath(LibraryManager::PluginsDir);
			fs::path path = mainpath;
			std::string procName = GetProcessName(), err;
			ctx = std::make_unique<PluginContext>(path, name, procName, err);

			if (!err.empty())
			{
				PX_LOG_ERROR(
					PX_MESSAGE("Failed to load plugin."),
					PX_LOGARG("Name", name),
					PX_LOGARG("Error", err)
				);
				return pl;
			}
		}

		pl = ctx->GetPlugin();
		pl->OnPluginPreLoad(this);

		if (!pl->OnPluginLoad(this))
		{
			PX_LOG_ERROR(
				PX_MESSAGE("Failed to load plugin, check it's log file."),
				PX_LOGARG("Name", ctx->GetFileName()),
			);
			return nullptr;
		}


		try
		{
			if (ctx->ShouldAutoLoad())
				ctx->LoadConfig();
			m_Plugins.emplace_back(std::move(ctx));
		}
		catch (const std::exception& ex)
		{
			PX_LOG_ERROR(
				PX_MESSAGE("Error while reading plugin's config."),
				PX_LOGARG("Name", name),
				PX_LOGARG("Exception", ex.what()),
			);
			return nullptr;
		}
	}

	for (auto& ctx : m_Plugins)
		ctx->OnPluginFullLoaded();

	return pl;
}

void DLLManager::LoadOneDLL(const std::string& proc_name, std::filesystem::path& path, const std::string& name)
{
	std::string err;
	auto ctx(std::make_unique<PluginContext>(path, name, proc_name, err));
	if (!err.empty())
	{
		PX_LOG_ERROR(
			PX_MESSAGE("Failed to load plugin."),
			PX_LOGARG("Name", name),
			PX_LOGARG("Error", err),
		);
	}
	else
		m_Plugins.emplace_back(std::move(ctx));
}

std::string DLLManager::GetProcessName()
{
#if BOOST_WINDOWS
	TCHAR proc_path[MAX_PATH]{ };
	DWORD size = static_cast<DWORD>(std::ssize(proc_path) - 1);
	QueryFullProcessImageName(GetCurrentProcess(), NULL, proc_path, &size);

	std::basic_string<TCHAR> procName(proc_path, size);
	return px::string_cast<std::string>(procName.substr(procName.find_last_of('\\') + 1));
#elif BOOST_LINUX
	extern char* program_invocation_short_name;
	return program_invocation_short_name;
#else
	return "any";
#endif
}

void DLLManager::UnloadPlugin_Internal(PluginContext* context)
{
	if (context->ShouldAutoSave())
		context->SaveConfig();

	// if the plugin is sharing an interface, erase it and call 'px::IPlugin::OnDropInterface' to notify other plugins
	std::erase_if(
		m_Interfaces,
		[this, pl = context->GetPlugin()](auto& it)
	{
		return it.second.Plugin == pl;
	});

	//px::console_manager.RemoveCommands(context->GetPlugin());

	std::erase_if(
		m_Plugins,
		[this, context](auto& other_ctx)
		{
			if (!other_ctx->HasDependencies())
				return false;

			bool erase = false;
			for (auto& pl_deps : other_ctx->GetDependencies())
			{
				if (pl_deps == context->GetPlugin())
				{
					this->UnloadPlugin_Internal(other_ctx.get());
					erase = true;
				}
			}
			return erase;
		}
	);
}

bool DLLManager::UnloadPlugin(const std::string& name)
{
	auto iter = FindContext(name);
	if (iter == m_Plugins.end())
		return false;

	UnloadPlugin_Internal(iter->get());
	m_Plugins.erase(iter);
	return true;
}

void DLLManager::UnloadAllDLLs()
{
	for (auto& ctx : m_Plugins)
	{
		if (ctx->ShouldAutoSave())
			ctx->SaveConfig();
	}

	// Erase plugins that depends on other plugins first
	std::erase_if(
		m_Plugins,
		[this] (auto& it)
		{
			return it->HasDependencies();
		}
	);

	// Erase external interfaces
	std::erase_if(
		m_Interfaces,
		[this](auto& it)
		{
			return it.second.Plugin != nullptr;
		}
	);

	//px::console_manager.RemoveCommands();
	m_Plugins.clear();

	px::logger.EndLogs();
}

px::IPlugin* DLLManager::FindPlugin(const std::string& name)
{
	auto iter = FindContext(name);
	return iter != m_Plugins.end() ? (*iter)->GetPlugin() : nullptr;
}

bool DLLManager::BindPlugin(px::IPlugin* caller, const char* name, bool is_required)
{
	px::IPlugin* pl = nullptr;
	
	auto caller_ctx = FindContext(caller);

	if (auto iter = FindContext(name); iter != m_Plugins.end())
	{
		pl = (*iter)->GetPlugin();
	}
	else if (is_required)
	{
		pl = LoadPlugin(name);
	}

	if (pl)
	{
		caller_ctx->GetDependencies().push_back(pl);
		return true;
	}
	else return false;
}

void DLLManager::RequestShutdown(px::IPlugin* pl)
{
	for (auto iter = m_Plugins.begin(); iter != m_Plugins.end(); iter++)
	{
		auto& ctx = *iter;
		if (ctx->GetPlugin() != pl)
			continue;

		UnloadPlugin_Internal(iter->get());
		m_Plugins.erase(iter);
		break;
	}
}

void DLLManager::UpdatePluginConfig(px::IPlugin* plugin, px::PlCfgLoadType loadtype)
{
	using px::PlCfgLoadType;
	const auto save_or_load =
		[this] (PluginContext* ctx, PlCfgLoadType loadtype)
	{
		switch (loadtype)
		{
		case PlCfgLoadType::Save:
		{
			ctx->SaveConfig();
			break;
		}
		case PlCfgLoadType::Load:
		{
			ctx->LoadConfig();
			break;
		}
		default: break;
		}
	};

	if (!plugin)
	{
		for (auto& ctx : m_Plugins)
			save_or_load(ctx.get(), loadtype);
	}
	else
	{
		for (auto& ctx : m_Plugins)
		{
			if (ctx->GetPlugin() != plugin)
				continue;

			save_or_load(ctx.get(), loadtype);
			break;
		}
	}
}