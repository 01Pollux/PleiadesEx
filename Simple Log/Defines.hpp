#pragma once


#define SG_PLUGIN_NAME			"Simple Log"
#define SG_PLUGIN_AUTHOR		"01Pollux"
#define SG_PLUGIN_DESC			"Logging Test"
#define SG_PLUGIN_VERSION		"1.0.0.0"


//#define SG_USING_PL_MANAGER		//	IPluginManager: PluginManager
//#define SG_USING_LIBRARY		//	ILibraryManager: LibManager
#define SG_USING_LOGGER			//	ILogger: Logger
//#define SG_USING_EVENT_MANAGER	//	IEventManager: EventManager
//#define SG_USING_DETOUR_MANAGER	//	IDetoursManager
#define SG_USING_IMGUI			//	IImGuiLoader
//#define SG_USING_PROFILER		//	Profiler::Manager



#include "dllImpl.hpp"

SG_NAMESPACE_BEGIN;

static constexpr PluginInfo ThisPluginInfo
{
	.m_Name = SG_PLUGIN_NAME,
	.m_Author = SG_PLUGIN_AUTHOR,
	.m_Description = SG_PLUGIN_DESC,
	.m_Date = __DATE__,
	.m_Version = Version(SG_PLUGIN_VERSION)
};

extern IPlugin* ThisPlugin;

SG_NAMESPACE_END;


#define SG_DLL_EXPORT(cls)													\
cls _dll_dummy_to_export;													\
SG_NAMESPACE_BEGIN;															\
IPlugin* ThisPlugin = static_cast<IPlugin*>(&_dll_dummy_to_export);			\
SG_NAMESPACE_END

#ifndef SG_USING_IMGUI
#define SG_TEXTMODE
#else
#define SG_GUIMODE
#endif
