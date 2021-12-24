#include <thread>
#include <fstream>
#include <mutex>

#include <shadowgarden/interfaces/PluginSys.hpp>
#include <shadowgarden/users/String.hpp>
#include <shadowgarden/users/StopWatch.hpp>

#include "Logger.hpp"
#include "Impl/Library/LibrarySys.hpp"


SG_NAMESPACE_BEGIN;

LoggerImpl logger;

struct LogSourceLoc
{
	uint_least32_t	Line{};
	std::string		File = "";
	std::string		Function = "";

	LogSourceLoc(const std::source_location& loc) noexcept :
		Line(loc.line()),
		File(loc.file_name()),
		Function(loc.function_name())
	{ }
};

struct LoggerImpl::InternalLoggerInfo
{
	std::string		FileName;
	std::string		TimeStamp;
	SG::PlLogType	LogType;
	LogSourceLoc	SourceLoc;
	nlohmann::json	Info;

	InternalLoggerInfo(SG::LoggerInfo&& info) noexcept;
};

LoggerImpl::InternalLoggerInfo::InternalLoggerInfo(SG::LoggerInfo&& linfo) noexcept :
	FileName(
		linfo.Plugin != ILogger::MainPlugin ?
		linfo.Plugin->GetFileName() :
		LibraryManager::MainLog
	),
	TimeStamp(FormatTime("{:%F %H:%M:%S}"sv)),
	LogType(linfo.LogType),
	SourceLoc(linfo.SourceLoc),
	Info(std::forward<nlohmann::ordered_json>(linfo.Info))
{ }

void LoggerImpl::LogMessage(LoggerInfo&& linfo)
{
	std::lock_guard guard(m_LogMutex);
	m_PendingLogs.emplace_front(std::forward<LoggerInfo>(linfo));
}

void LoggerImpl::StartLogs()
{
	m_LogTimer = std::jthread(
		[] (std::stop_token tok, LoggerImpl* pLogger)
	{
		std::stop_callback callback(
			tok,
			[pLogger] ()
		{
			pLogger->WriteLogs();
		});

		while (true)
		{
			std::this_thread::sleep_for(SleepDuration);

			if (tok.stop_requested())
				return;

			pLogger->WriteLogs();
		}
	}, this);
}

void LoggerImpl::WriteLogs()
{
	std::lock_guard guard(m_LogMutex);

	if (m_PendingLogs.empty())
		return;

	std::map<std::string, nlohmann::ordered_json> final_info;

	for (auto& linfo : m_PendingLogs)
	{
		const char* stype = "";
		switch (linfo.LogType)
		{
		case PlLogType::Dbg:
			stype = "Debug";
			break;
		case PlLogType::Msg:
			stype = "Message";
			break;
		case PlLogType::Err:
			stype = "Error";
			break;
		case PlLogType::Ftl:
			stype = "Fatal";
			break;
		}

		nlohmann::ordered_json res
		{
			{
				stype,
				{
					{
						linfo.TimeStamp,
						{
							SG_LOGARG("File",		std::move(linfo.SourceLoc.File)),
							SG_LOGARG("Function",	std::move(linfo.SourceLoc.Function)),
							SG_LOGARG("Line",		linfo.SourceLoc.Line),
							SG_LOGARG("Info",		std::move(linfo.Info)),
						}
					}
				}
			}
		};
		
		final_info[linfo.FileName].merge_patch(std::move(res));
	}

	for (auto& [name, data] : final_info)
	{
		std::string file_name = std::format(
			"{0}\\{1}.log"sv,
			LibraryManager::LogsDir,
			name
		);

		{
			std::ifstream in_file(file_name, std::ios::ate);
			if (in_file && in_file.tellg() > 0)
			{
				in_file.seekg(0, in_file.beg);
				data.merge_patch(nlohmann::ordered_json::parse(in_file, nullptr, true, true));
			}
		}

		{
			std::ofstream out_file(file_name, std::ios::trunc);
			out_file.width(4);
			out_file << data;
		}
	}

	m_PendingLogs.clear();
}

void LoggerImpl::EndLogs()
{
	m_LogTimer.request_stop();
	// TODO: fix std::system_error no such process when detaching and attaching dll very fast
	if (m_LogTimer.joinable())	m_LogTimer.join();
}

SG_NAMESPACE_END;