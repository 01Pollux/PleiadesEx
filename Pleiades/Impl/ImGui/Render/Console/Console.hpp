#pragma once

#include <filesystem>
#include <fstream>

#include <imgui/imgui.h>

#include "Impl/Console/config.hpp"
#include "../render.hpp"

SG_NAMESPACE_BEGIN;

class ImGui_Console
{
	friend class ConsoleManager;
public:
	struct LogInfo
	{
		uint32_t Color;
		std::string Text;
	};

	void Render();

private:

	std::string m_Input;
	std::vector<LogInfo> m_Logs;
	std::vector<std::string> m_HistoryCmds;
	std::vector<ConCommand*> m_Commands;

	ptrdiff_t m_HistoryPos{ -1 };
};

extern ImGui_Console imgui_console;

SG_NAMESPACE_END;