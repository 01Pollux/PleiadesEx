#pragma once

#include <filesystem>
#include <fstream>

#include <imgui/imgui.h>

#include "console/Manager.hpp"
#include "imgui/frontends/renderer.hpp"

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
	void ClearHistory();
	void InsertToHistory();
	void ClearInput()
	{
		m_Input.clear();
	}
	[[nodiscard]] bool InputContaints(px::ConCommand* cmd);

	int static OnEditTextCallback(ImGuiInputTextCallbackData* pData);

	std::string					m_Input;
	std::vector<LogInfo>		m_Logs;
	std::vector<std::string>	m_HistoryCmds;
	std::vector<px::ConCommand*>m_Commands;

	std::string_view			m_SuggestedCommand;
	ptrdiff_t	m_HistoryPos{ -1 };
};

PX_NAMESPACE_BEGIN();
inline ImGui_Console imgui_console;
PX_NAMESPACE_END();
