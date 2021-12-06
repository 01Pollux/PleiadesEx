#pragma once

#include <thread>
#include <mutex>
#include "Interfaces/Logger.hpp"


SG_NAMESPACE_BEGIN

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

	std::list<InternalLoggerInfo>		m_PendingLogs;
	std::jthread _LogTimer;
	std::mutex	 _LogMutex;
};

extern LoggerImpl logger;

SG_NAMESPACE_END;

#ifndef SG_NO_LOGGERS

#undef SG_INTERNAL_LOGGER_INST
#define SG_INTERNAL_LOGGER_INST		(&SG::logger)
#undef SG_INTERNAL_LOGGER_PL
#define SG_INTERNAL_LOGGER_PL		SG::ILogger::MainPlugin

#endif