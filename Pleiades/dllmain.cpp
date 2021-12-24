#include <boost/config.hpp>

#include <shadowgarden/users/Profiler.hpp>
#include <shadowgarden/users/Modules.hpp>
#include <shadowgarden/users/String.hpp>
#include <shadowgarden/users/Types.hpp>

#include "Impl/Plugins/PluginManager.hpp"
#include "Impl/Library/LibrarySys.hpp"
#include "Impl/Interfaces/Logger.hpp"
#include "Impl/ImGui/imgui_iface.hpp"

#if BOOST_WINDOWS
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <minidumpapiset.h>
#include <mmsystem.h>
static PVOID g_ExceptionHandler;
#endif

static void* mainModule;

#if BOOST_WINDOWS
static LONG NTAPI OnRaiseException(_In_ PEXCEPTION_POINTERS ExceptionInfo);
#endif

extern "C" BOOST_SYMBOL_EXPORT bool
Tella_LoadDLL(void* hMod)
{
#if BOOST_WINDOWS
    g_ExceptionHandler = AddVectoredExceptionHandler(1, OnRaiseException);
#endif

	mainModule = hMod;
	SG::Profiler::Manager::Alloc();

	SG::logger.StartLogs();

	SG::lib_manager.BuildDirectories();

	if (!SG::plugin_manager.BasicInit())
	{
        RemoveVectoredExceptionHandler(g_ExceptionHandler);
        SG::Profiler::Manager::Release();

		SG_LOG_FATAL(
			SG_MESSAGE("Failed to load main plugin, check for any missing configs.")
		);
		return false;
	}

	return true;
}

static SG::DWord
BOOST_WINAPI_DETAIL_STDCALL
Tella_EjectDLL(void* hMod)
{
	SG::plugin_manager.UnloadAllDLLs();

    SG::Profiler::Manager::Release();
    
    SG::plugin_manager.BasicShutdown();

    RemoveVectoredExceptionHandler(g_ExceptionHandler);

#if BOOST_WINDOWS
	FreeLibraryAndExitThread(std::bit_cast<HMODULE>(hMod), EXIT_SUCCESS);
#endif

    return true;
}


void SG::DLLManager::Shutdown() noexcept
{
#if BOOST_WINDOWS
	HANDLE hThread = CreateThread(nullptr, NULL, Tella_EjectDLL, mainModule, NULL, nullptr);
	if (hThread)
		CloseHandle(hThread);
#else
	std::thread([] () { Tella_EjectDLL(mainModule); }).detach();
#endif
}


#if BOOST_WINDOWS

LONG NTAPI OnRaiseException(_In_ PEXCEPTION_POINTERS ExceptionInfo)
{
    std::unique_ptr<void, decltype([](void* handle){ FreeLibrary(std::bit_cast<HMODULE>(handle)); })> DBGHelp{ std::bit_cast<void*>(LoadLibrary(TEXT("DBGHelp.dll"))) };
    if (!DBGHelp)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_INVALID_HANDLE:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_STACK_INVALID:
    case EXCEPTION_WRITE_FAULT:
    case EXCEPTION_READ_FAULT:
    case STATUS_STACK_BUFFER_OVERRUN:
    case STATUS_HEAP_CORRUPTION:
        break;
    case EXCEPTION_BREAKPOINT:
        if (!(ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE))
            return EXCEPTION_CONTINUE_SEARCH;
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }

    using MiniDumpWriteDumpFn =
        BOOL(WINAPI*)(
            _In_ HANDLE hProcess,
            _In_ DWORD ProcessId,
            _In_ HANDLE hFile,
            _In_ MINIDUMP_TYPE DumpType,
            _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
            _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
            _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
            );

    HANDLE hFile = CreateFileW(
        SG::FormatTime(L"./Pleiades/Logs/Fatal_{0:%g}_{0:%h}_{0:%d}_{0:%H}_{0:%OM}_{0:%OS}.dmp").c_str(),
        GENERIC_WRITE,
        NULL, 
        NULL, 
        CREATE_NEW, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return EXCEPTION_CONTINUE_SEARCH;

    MiniDumpWriteDumpFn WriteMiniDump = std::bit_cast<MiniDumpWriteDumpFn>(GetProcAddress(std::bit_cast<HMODULE>(DBGHelp.get()), "MiniDumpWriteDump"));

    MINIDUMP_EXCEPTION_INFORMATION MiniDumpInfo{
           .ThreadId = GetCurrentThreadId(),
           .ExceptionPointers = ExceptionInfo,
           .ClientPointers = FALSE };

    BOOL res = WriteMiniDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        static_cast<MINIDUMP_TYPE>(MiniDumpWithUnloadedModules | MiniDumpWithFullMemoryInfo | MiniDumpWithCodeSegs),
        &MiniDumpInfo,
        NULL,
        NULL
    );

    CloseHandle(hFile);

    if (res)
    {
        ExceptionInfo->ExceptionRecord->ExceptionCode = EXCEPTION_BREAKPOINT;
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else return EXCEPTION_CONTINUE_SEARCH;
}

#endif
