
#include <filesystem>
#include <fstream>

#include "Logger.hpp"
#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Interfaces/Logger.hpp"

SG_NAMESPACE_BEGIN;

void ImGui_BrdigeRenderer::RenderLogger()
{
	static ImGui_JsonLogger manager;

	if (ImGui::Button(ICON_FA_REDO " Refresh"))
		manager.LoadLogs();

	manager.DrawLoggerDesign();
}

void ImGui_JsonLogger::LoadLogs()
{
	m_LogSection.Plugins.clear();
	char path[MAX_PATH];

	if (!SG::lib_manager.GoToDirectory(SG::PlDirType::Logs, nullptr, path, std::ssize(path)))
	{
		SG_LOG_MESSAGE(
			SG_MESSAGE("Exception reported while loading Packs"),
			SG_LOGARG("Exception", path)
		);
		m_LogSection.reset();
		return;
	}

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

	m_LogSection.Plugins.shrink_to_fit();
	m_LogSection.reset();
}

void ImGui_JsonLogger::DrawLoggerDesign()
{
	if (ImGui::BeginChild("Current Logs", { 200.f, 0.f }, true, ImGuiWindowFlags_HorizontalScrollbar))
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

		ImGui::EndChild();
		ImGui::SameLine();
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	ImGui::BeginGroup();
	{
		if (ImGui::BeginChild("Current Configs", { 0, 0.f }, false, ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (m_LogSection.Current != m_LogSection.Plugins.end())
			{
				ImGuiJsLogInfo& plinfo = *m_LogSection.Current;
				plinfo.DrawPopupState();
				plinfo.DrawLogs(m_LogSection.Filter);
			}
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}


SG_NAMESPACE_END;