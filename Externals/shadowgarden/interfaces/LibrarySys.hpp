#pragma once

#include "User/IntPtr.hpp"
#include "InterfacesSys.hpp"

_STD_BEGIN;

template<typename>
class function;

_STD_END;

namespace asmjit
{
	class JitRuntime;
}

SG_NAMESPACE_BEGIN;

namespace Profiler
{
	class Manager;
}

class IGameData;

static constexpr const char* Interface_ILibrary = "ILibraryManager";

class ILibrary : public IInterface
{
public:
	/// <summary>
	/// read a signature from pattern
	/// </summary>
	/// <returns>pointer to the target address, null if it doesn't exists</returns>
	virtual IntPtr FindByName(const std::string& name) abstract;

	/// <summary>
	/// read a signature from pattern
	/// </summary>
	/// <returns>pointer to the target address, null if it doesn't exists</returns>
	virtual IntPtr FindBySignature(const std::string& signature) abstract;

	using SignaturePredicate = std::function<bool(IntPtr cur_address, IntPtr end_address)>;
	/// <summary>
	/// read a signature from predicate function
	/// </summary>
	/// <returns>pointer to the target address, null if it doesn't exists</returns>
	virtual IntPtr FindBySignature(const SignaturePredicate& signature) abstract;

	/// <summary>
	/// read a signature from a string
	/// </summary>
	/// <returns>pointer to the target address, null if it doesn't exists</returns>
	virtual IntPtr FindByString(const std::string& str) abstract;
};

enum class PlDirType
{
	Root,	// main executable directory
	Main,	// ./Pleiades/ directory
	Plugins,// ./Pleiades/Plugins
	Logs,	// ./Pleiades/Logs
	Profiler,//./Pleiades/Logs/Profiler
	Sounds	// ./Pleiades/Sounds
};

class ILibraryManager : public IInterface
{
public:
	static constexpr const char* MainName	= "Pleiades";
	static constexpr const char* MainDir	= "Pleiades";
	static constexpr const char* MainLog	= "Pleiades";
	static constexpr const char* MainCfg	= "Pleiades\\Pleiades.json";
	static constexpr const char* CommonTag	= "Pleiades\\Pleiades";
	static constexpr const char* PluginsDir = "Pleiades\\Plugins";
	static constexpr const char* SoundsDir	= "Pleiades\\Sounds";
	static constexpr const char* LogsDir	= "Pleiades\\Logs";
	static constexpr const char* ProfilerDir= "Pleiades\\Logs\\Profiler";

	/// <summary>
	/// read a pre-loaded library
	/// </summary>
	/// <returns>pointer to the module, null if it doesn't exists</returns>
	virtual ILibrary* ReadLibrary(const char* module_name) abstract;

	/// <summary>
	/// load a module to the process
	/// </summary>
	/// <param name="load_in_memory">Load dll without LoadLibraryA</param>
	/// <returns>pointer to the module, null if it doesn't exists</returns>
	virtual ILibrary* OpenLibrary(const char* module_name) abstract;

	/// <summary>
	/// load a module to the process
	/// </summary>
	/// <param name="load_in_memory">Load dll without LoadLibraryA</param>
	/// <returns>pointer to the module, null if it doesn't exists</returns>
	virtual ILibrary* OpenLibrary(const wchar_t* module_name) abstract;

	/// <summary>
	/// Get desired directory from type
	/// </summary>
	/// <param name="type">directory type</param>
	/// <param name="dir">extra directory to travel to</param>
	/// <param name="path">output buffer</param>
	/// <param name="max_len">buffer length</param>
	/// <returns>true if directory exists, false otherwise, exception output will be displayed in output buffer 'path'</returns>
	virtual bool GoToDirectory(PlDirType type, const char* dir, char* path, size_t max_len) abstract;

	/// <summary>
	/// Open gamedata for the plugin '<PLUGIN_NAME>.<GAME_NAME>.gamedata', or null to open 'Pleiades.<GAME_NAME>.gamedata'
	/// </summary>
	/// <returns>pointer to the gamedata, and must be delete afterwards</returns>
	virtual IGameData* OpenGameData(IPlugin* pPlugin) abstract;

	/// <summary>
	/// Get current host name
	/// </summary>
	/// <returns></returns>
	virtual const std::string& GetHostName() abstract;

	/// <summary>
	/// Get AsmJit main runtime
	/// </summary>
	/// <returns></returns>
	virtual asmjit::JitRuntime* GetRuntime() abstract;

	/// <summary>
	/// Get last error for current os
	/// </summary>
	virtual std::string GetLastError() abstract;

	/// <summary>
	/// Get main profiler instance
	/// </summary>
	virtual Profiler::Manager* GetProfiler() abstract;
};

SG_NAMESPACE_END;