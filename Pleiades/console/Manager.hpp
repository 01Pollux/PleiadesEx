#pragma once

#include <px/console.hpp>

class ConsoleManager : public px::IConsoleManager
{
protected:
	/// <summary>
	/// Add console commands to plugin's commands
	/// </summary>
	bool AddCommands(px::ConCommand* command) override;

public:
	bool RemoveCommand(px::ConCommand* command) override;
	
	bool RemoveCommands(px::IPlugin* plugin) override;

	void RemoveCommands();
	
	px::ConCommand* FindCommand(const std::string_view& name) override;
	
	std::vector<px::ConCommand*> FindCommands(px::IPlugin* plugin) override;

	std::vector<px::ConCommand*> FindCommands(const std::string_view& name) override;

	void Execute(const std::string_view& cmds) override;

	void Clear(size_t size = 0, bool is_history = false) override;

	void Print(uint32_t color, const std::string& msg) override;

	void Print(const std::array<uint8_t, 4>& color, const std::string& msg)
	{
		Print(
			color[0] | color[1] << 0x8 | color[2] << 0x10 | color[3] << 0x18,
			msg
		);
	}

	void Print(const std::string& msg)
	{
		Print(
			255 | 255 << 0x8 | 255 << 0x10 | 255 << 0x18,
			msg
		);
	}
};

PX_NAMESPACE_BEGIN();
inline ConsoleManager console_manager;
PX_NAMESPACE_END();
