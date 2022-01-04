#pragma once


#define PX_PLUGIN_NAME			"ImGui Interface"
#define PX_PLUGIN_AUTHOR		"01Pollux"
#define PX_PLUGIN_DESC			"Configure interface with ImGui GH:'ocornut/imgui' and ImPlot GH:'epezent/implot'"
#define PX_PLUGIN_VERSION		"1.31.1.0"


#define PX_USING_PL_MANAGER		//	IPluginManager: PluginManager
#define PX_USING_LIBRARY		//	ILibraryManager: LibManager
#define PX_USING_LOGGER			//	ILogger: Logger
//#define PX_USING_EVENT_MANAGER	//	IEventManager: EventManager
#define PX_USING_DETOUR_MANAGER	//	IDetoursManager
//#define PX_USING_IMGUI			//	IImGuiLoader
#define PX_USING_PROFILER		//	profiler::manager



#include "dllImpl.hpp"

PX_NAMESPACE_BEGIN();

static constexpr PluginInfo ThisPluginInfo
{
	.m_Name = PX_PLUGIN_NAME,
	.m_Author = PX_PLUGIN_AUTHOR,
	.m_Description = PX_PLUGIN_DESC,
	.m_Date = __DATE__,
	.m_Version = Version(PX_PLUGIN_VERSION)
};

extern IPlugin* ThisPlugin;

PX_NAMESPACE_END();


#define PX_DLL_EXPORT(cls)													\
cls _dll_dummy_to_export;													\
PX_NAMESPACE_BEGIN();															\
IPlugin* ThisPlugin = static_cast<IPlugin*>(&_dll_dummy_to_export);			\
PX_NAMESPACE_END()

#ifndef PX_USING_IMGUI
#define PX_TEXTMODE
#else
#define PX_GUIMODE
#endif
