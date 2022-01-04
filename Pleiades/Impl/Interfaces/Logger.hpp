#pragma once

#include <thread>
#include <mutex>
#include <px/interfaces/Logger.hpp>

PX_NAMESPACE_BEGIN();

class LoggerImpl : public ILogger
{
public:
	static constexpr auto SleepDuration = std::chrono::seconds(1);

	// Inherited via ILogger
	void LogMessage(LoggerInfo&& linfo) override;

	void StartLogs();
	void WriteLogs();
	void EndLogs();

private:
	struct InternalLoggerInfo;

	std::list<InternalLoggerInfo> m_PendingLogs;
	std::jthread m_LogTimer;
	std::mutex	 m_LogMutex;
};

extern LoggerImpl logger;

PX_NAMESPACE_END();

#ifndef PX_NO_LOGGERS

#undef PX_INTERNAL_LOGGER_INST
#define PX_INTERNAL_LOGGER_INST		(&px::logger)
#undef PX_INTERNAL_LOGGER_PL
#define PX_INTERNAL_LOGGER_PL		px::ILogger::MainPlugin

#endif