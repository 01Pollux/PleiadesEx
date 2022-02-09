#pragma once

#include <px/console.hpp>
#include "imgui/frontends/console/Console.hpp"

class ConsoleManager : public px::IConsoleManager
{
public:
	bool RemoveCommand(px::con_command* command) override;
	
	bool RemoveCommands(px::IPlugin* plugin) override;

	[[nodiscard]] const std::vector<ImGui_Console::CmdWrapper>& GetCommands() const noexcept;

	std::vector<px::con_command*> FindCommands(px::IPlugin* plugin) override;
	
	px::con_command* FindCommand(std::string_view name) override;

	void Execute(std::string_view cmds) override;

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

	void AddCommands()
	{
		AddCommands(nullptr, px::cmd_manager::begin(), px::cmd_manager::end());
		px::cmd_manager::clear();
	}

protected:
	/// <summary>
	/// Add console commands to plugin's commands
	/// </summary>
	void AddCommands(
		px::IPlugin* plugin,
		px::con_command::entries_type::iterator begin,
		px::con_command::entries_type::iterator end
	) override;
};

PX_NAMESPACE_BEGIN();
inline ConsoleManager console_manager;
PX_NAMESPACE_END();
