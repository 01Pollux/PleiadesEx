#pragma once

#include <format>
#include <source_location>
#include <nlohmann/Json.hpp>

#include "Interfaces/InterfacesSys.hpp"


SG_NAMESPACE_BEGIN;

static constexpr const char* Interface_ILogger = "ILogger";

class IPlugin;

enum class PlLogType : unsigned char
{
	Msg,
	Dbg,
	Err,
	Ftl
};

struct LoggerInfo
{
	IPlugin* Plugin;
	PlLogType	LogType;
	const std::source_location SourceLoc;
	OrderedJson	Info;

	LoggerInfo(IPlugin* pl, PlLogType ltype, const std::source_location loc, OrderedJson&& msg) noexcept :
		Plugin(pl), LogType(ltype), SourceLoc(loc), Info(std::forward<OrderedJson>(msg))
	{ }
};

class ILogger : public IInterface
{
public:
	static constexpr IPlugin* MainPlugin = nullptr;
	virtual void LogMessage(LoggerInfo&& linfo) abstract;

	void DbgEx(IPlugin* plugin, std::source_location loc, OrderedJson&& info)
	{
		LogMessage({ plugin, PlLogType::Dbg, loc, std::forward<OrderedJson>(info) });
	}

	void MsgEx(IPlugin* plugin, std::source_location loc, OrderedJson&& info)
	{
		LogMessage({ plugin, PlLogType::Msg, loc, std::forward<OrderedJson>(info) });
	}
	void ErrEx(IPlugin* plugin, std::source_location loc, OrderedJson&& info)
	{
		LogMessage({ plugin, PlLogType::Err, loc, std::forward<OrderedJson>(info) });
	}

	void FtlEx(IPlugin* plugin, std::source_location loc, OrderedJson&& info)
	{
		LogMessage({ plugin, PlLogType::Ftl, loc, std::forward<OrderedJson>(info) });
	}
};

SG_NAMESPACE_END;


#ifndef SG_NO_LOGGERS
	#define SG_INTERNAL_LOGGER_INST		SG::Logger

	#define SG_INTERNAL_LOGGER_PL		SG::ThisPlugin

	#define SG_LOGARG(name, ...)		{ name, __VA_ARGS__ }

	#define SG_MESSAGE(msg)				SG_LOGARG("Message", msg)

	#define SG_BEGIN_LOGCTX(...)		do { \
										constexpr std::source_location log_loc(std::source_location::current()); OrderedJson log_ctx{ __VA_ARGS__ }

	#define SG_SET_LOGCTX(name, ...)	log_ctx[name] = __VA_ARGS__

	#define SG_GET_LOGCTX				log_ctx

	#define SG_END_LOGCTX(TYPE, PL)		SG_INTERNAL_LOGGER_INST->TYPE##Ex(PL, log_loc, std::move(SG_GET_LOGCTX));\
										} while (false)

	#ifdef SG_DEBUG
		#define SG_LOG_DEBUG(...)							\
				SG_BEGIN_LOGCTX(__VA_ARGS__);				\
				SG_END_LOGCTX(Dbg, SG_INTERNAL_LOGGER_PL)

	#else
		#define SG_LOG_DEBUG(...)
	#endif

	#define SG_LOG_MESSAGE(...)								\
			SG_BEGIN_LOGCTX(__VA_ARGS__);					\
			SG_END_LOGCTX(Msg, SG_INTERNAL_LOGGER_PL)																

	#define SG_LOG_ERROR(...)								\
			SG_BEGIN_LOGCTX(__VA_ARGS__);					\
			SG_END_LOGCTX(Msg, SG_INTERNAL_LOGGER_PL)																

	#define SG_LOG_FATAL(...)								\
			SG_BEGIN_LOGCTX(__VA_ARGS__);					\
			SG_END_LOGCTX(Msg, SG_INTERNAL_LOGGER_PL)																	

#else
	#define SG_INTERNAL_LOGGER_INST

	#define SG_INTERNAL_LOGGER_PL

	#define SG_LOGARG(name, ...)

	#define SG_MESSAGE(msg)

	#define SG_BEGIN_LOGCTX(data)	

	#define SG_SET_LOGINFO(name, ...) 

	#define SG_LOG_CTX	

	#define SG_END_LOGCTX

	#define SG_LOG_DEBUG(...)

	#define SG_LOG_MESSAGE(...)

	#define SG_LOG_ERROR(...) 

	#define SG_LOG_FATAL(...)
#endif