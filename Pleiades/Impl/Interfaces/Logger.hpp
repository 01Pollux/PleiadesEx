#pragma once

#include <thread>
#include <mutex>
#include <shadowgarden/interfaces/Logger.hpp>

SG_NAMESPACE_BEGIN;

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

SG_NAMESPACE_END;

#ifndef SG_NO_LOGGERS

#undef SG_INTERNAL_LOGGER_INST
#define SG_INTERNAL_LOGGER_INST		(&SG::logger)
#undef SG_INTERNAL_LOGGER_PL
#define SG_INTERNAL_LOGGER_PL		SG::ILogger::MainPlugin

#endif