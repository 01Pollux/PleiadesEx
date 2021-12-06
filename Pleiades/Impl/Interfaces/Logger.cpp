#include <thread>
#include <fstream>
#include <mutex>

#include "Logger.hpp"
#include "User/String.hpp"
#include "User/StopWatch.hpp"

#include "Interfaces/PluginSys.hpp"
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
	Json			Info;

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
	Info(std::forward<OrderedJson>(linfo.Info))
{ }

void LoggerImpl::LogMessage(LoggerInfo&& linfo)
{
	std::lock_guard guard(_LogMutex);
	m_PendingLogs.emplace_front(std::forward<LoggerInfo>(linfo));
}

void LoggerImpl::StartLogs()
{
	_LogTimer = std::jthread(
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
	std::lock_guard guard(_LogMutex);

	if (m_PendingLogs.empty())
		return;

	std::map<std::string, OrderedJson> final_info;

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

		OrderedJson res
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
				data.merge_patch(OrderedJson::parse(in_file, nullptr, true, true));
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
	_LogTimer.request_stop();
	// TODO: fix std::system_error no such process when detaching and attaching dll very fast
	if (_LogTimer.joinable())	_LogTimer.join();
}

SG_NAMESPACE_END;