#pragma once

#include <shadowgarden/config.hpp>

SG_NAMESPACE_BEGIN;

class ConsoleManager : public IConsoleManager
{
protected:
	/// <summary>
	/// Add console commands to plugin's commands
	/// </summary>
	bool AddCommands(ConCommand* command) override;

public:
	bool RemoveCommand(ConCommand* command) override;
	
	bool RemoveCommands(IPlugin* plugin) override;
	
	ConCommand* FindCommand(const std::string_view& name) override;
	
	std::vector<ConCommand*> FindCommands(IPlugin* plugin) override;
	
	void Print(const std::array<uint8_t, 4>& color, const std::string& msg) override;
};
extern ConsoleManager console_manager;

SG_NAMESPACE_END;