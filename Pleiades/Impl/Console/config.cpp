#include "Impl/Plugins/PluginManager.hpp"
#include "Impl/ImGui/Render/Console/Console.hpp"
#include "config.hpp"

SG_NAMESPACE_BEGIN;

ConsoleManager console_manager;

bool ConsoleManager::AddCommands(ConCommand* command)
{
	PluginContext* pCtx = SG::plugin_manager.FindContext(command->plugin());
	if (!pCtx)
		return false;

	ConCommand* cmd = command;
	while (cmd)
	{
		imgui_console.m_Commands.push_back(cmd);
		cmd = std::exchange(cmd->m_NextCommand, nullptr);
	}

	return true;
}

bool ConsoleManager::RemoveCommand(ConCommand* command)
{
	PluginContext* pCtx = SG::plugin_manager.FindContext(command->plugin());
	if (!pCtx)
		return false;

	return std::erase(
		imgui_console.m_Commands,
		command
	) != 0;
}

bool ConsoleManager::RemoveCommands(IPlugin* plugin)
{
	PluginContext* pCtx = SG::plugin_manager.FindContext(plugin);
	if (!pCtx)
		return false;

	return std::erase_if(
		imgui_console.m_Commands,
		[plugin](ConCommand* cmd)
		{
			return cmd->plugin() == plugin;
		}
	) != 0;
}

ConCommand* ConsoleManager::FindCommand(const std::string_view& name)
{
	for (ConCommand* cmd : imgui_console.m_Commands)
	{
		if (name == cmd->name())
			return cmd;
	}
	return nullptr;
}

std::vector<ConCommand*> ConsoleManager::FindCommands(IPlugin* plugin)
{
	std::vector<ConCommand*> cmds;
	for (ConCommand* cmd : imgui_console.m_Commands)
	{
		if (plugin == cmd->plugin())
			cmds.push_back(cmd);
	}
	return cmds;
}

void ConsoleManager::Print(const std::array<uint8_t, 4>& color, const std::string& msg)
{
	imgui_console.m_Logs.emplace_back(
		// R | G << 0x8 | B << 0x10 | A << 0x18
		color[0] | color[1] << 0x8 | color[2] << 0x10 | color[3] << 0x18,
		msg
	);
}

SG_NAMESPACE_END;