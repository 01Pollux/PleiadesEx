#pragma once

#include "../render.hpp"
#include "imgui_iface.hpp"

SG_NAMESPACE_BEGIN;

struct ImGuiJsLogInfo
{
	std::string FileName;
	Json Logs;

	/// <summary>
	/// Draw popup states
	/// 
	/// TODO: 
	/// 'Refresh': for clearing logs and reinserting new ones
	/// 'Clear': for clearing
	/// 'View': Open Text editor to view raw json file
	/// 'Copy': Copy to clipboard, either every message or a single one
	/// 'Paste': Paste from clipboard, either every message or a single one
	/// 'Erase': Delete a single section
	/// 
	/// </summary>
	void DrawPopupState();

	/// <summary>
	/// Draw nested logs information
	/// </summary>
	void DrawLogs(const ImGuiTextFilter& filter);

	/// <summary>
	/// Filter commands for current entry
	/// </summary>
	/// <param name="is_type">true to only process '/t=' command, false otherwise</param>
	/// <returns></returns>
	bool FilterInfo(const ImGuiTextFilter& filter, Json::const_iterator iter, bool is_type) const;
};

struct ImGuiPlLogSection
{
	using container_type = std::vector<ImGuiJsLogInfo>;
	using iterator_type = container_type::iterator;

	container_type Plugins;
	ImGuiTextFilter Filter;
	iterator_type Current{ Plugins.end() };

	void reset() noexcept
	{
		Current = Plugins.end();
	}

	bool FilterPlugin(iterator_type iter) const;
};


class ImGui_JsonLogger
{
public:
	ImGui_JsonLogger()
	{
		LoadLogs();
	}

	/// <summary>
	/// Clear 'm_PlManSection.Plugins' and re-insert logs
	/// </summary>
	void LoadLogs();

	/// <summary>
	/// Draw logs' design and filter
	/// </summary>
	void DrawLoggerDesign();

private:
	ImGuiPlLogSection m_LogSection;
};

SG_NAMESPACE_END;