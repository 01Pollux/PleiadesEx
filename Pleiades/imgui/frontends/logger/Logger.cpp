
#include <filesystem>
#include <fstream>

#include "Logger.hpp"
#include "library/Manager.hpp"
#include "logs/Logger.hpp"


void ImGui_JsonLogger::LoadLogs()
{
	m_LogSection.Plugins.clear();

	try
	{
		
		if (std::string path = px::lib_manager.GoToDirectory(px::PlDirType::Logs); !path.empty())
		{
			namespace fs = std::filesystem;
			for (auto& dir : fs::directory_iterator(path))
			{
				if (dir.path().extension() != ".log")
					continue;

				std::ifstream file(dir.path());
				auto log{ nlohmann::json::parse(file, nullptr, false, true) };
				if (log.is_discarded())
					continue;

				m_LogSection.Plugins.emplace_back(dir.path().stem().string(), std::move(log));
			}
		}
	}
	catch (const std::exception& ex)
	{
		PX_LOG_MESSAGE(
			PX_MESSAGE("Exception reported while loading Packs"),
			PX_LOGARG("Exception", ex.what())
		);
		m_LogSection.reset();
		return;
	}
	
	m_LogSection.Plugins.shrink_to_fit();
	m_LogSection.reset();
}

void ImGui_JsonLogger::DrawLoggerDesign()
{
	if (imcxx::window_child cur_logs{ "Current Logs", { 200.f, 0.f }, true, ImGuiWindowFlags_HorizontalScrollbar })
	{
		ImGui::SameLine(); 
		m_LogSection.Filter.Draw("", -25.f);
		ImGui::SameLineHelp(
			"Filter: (inc, -exc)\n"
			"'/p=': plugin's name\n"
			"'/t=': Message type (debug, message, error, fatal)\n"
			"'/d=[<=>]': Log date\n"
			"'/e=': Exception text\n"
			"'/m=': Message text\n"
		);

		for (auto iter = m_LogSection.Plugins.begin(); iter != m_LogSection.Plugins.end(); iter++)
		{
			if (!m_LogSection.FilterPlugin(iter))
				continue;

			const bool selected = m_LogSection.Current == iter;

			if (ImGui::Selectable(iter->FileName.c_str(), selected))
				m_LogSection.Current = iter;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

	}

	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	imcxx::group cur_configs;

	if (imcxx::window_child cur_logs{ "Current Configs", { 0, 0.f }, false, ImGuiWindowFlags_HorizontalScrollbar })
	{
		if (m_LogSection.Current != m_LogSection.Plugins.end())
		{
			ImGuiJsLogInfo& plinfo = *m_LogSection.Current;
			plinfo.DrawPopupState();
			plinfo.DrawLogs(m_LogSection.Filter);
		}
	}
}
