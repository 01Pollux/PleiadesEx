
#include <memory>
#include <format>
#include <chrono>

#include <px/profiler.hpp>

#include "library/Manager.hpp"
#include "plugins/Manager.hpp"
#include "detours/HooksManager.hpp"
#include "Logs/Logger.hpp"

#include <Windows.h>
#include <minidumpapiset.h>

static PVOID g_ExceptionHandler;
static PVOID g_hModule;

LONG NTAPI OnRaiseException(_In_ PEXCEPTION_POINTERS ExceptionInfo);

extern "C" __declspec(dllexport) bool __cdecl
Tella_LoadDLL(void* hMod)
{
	CreateThread(
		nullptr,
		0,
		[](LPVOID hMod) -> DWORD
		{
			g_ExceptionHandler = AddVectoredExceptionHandler(1, OnRaiseException);

			px::profiler::manager::Alloc();
			px::logger.StartLogs();

			px::lib_manager.BuildDirectories();

			if (!px::plugin_manager.BasicInit())
			{
				px::profiler::manager::Release();
				RemoveVectoredExceptionHandler(g_ExceptionHandler);
				PX_LOG_FATAL(
					PX_MESSAGE("Failed to load main plugin, check for any missing configs.")
				);

				FreeLibraryAndExitThread(std::bit_cast<HMODULE>(hMod), EXIT_SUCCESS);
			}
			else g_hModule = hMod;
			return EXIT_SUCCESS;
		},
		hMod,
		0,
		nullptr
	);
	return true;
}


void DLLManager::Shutdown() noexcept
{
	CreateThread(
		nullptr,
		0,
		[](LPVOID) -> DWORD
		{
			px::plugin_manager.UnloadAllDLLs();

			px::profiler::manager::Release();

			px::plugin_manager.BasicShutdown();

			px::detour_manager.SleepToReleaseHooks();

			RemoveVectoredExceptionHandler(g_ExceptionHandler);

			FreeLibraryAndExitThread(std::bit_cast<HMODULE>(g_hModule), EXIT_SUCCESS);
			
			return EXIT_SUCCESS;
		},
		nullptr,
		0,
		nullptr
	);
}


LONG NTAPI OnRaiseException(_In_ PEXCEPTION_POINTERS ExceptionInfo)
{
	std::unique_ptr<void, decltype([](void* handle) { FreeLibrary(std::bit_cast<HMODULE>(handle)); })> DBGHelp{ std::bit_cast<void*>(LoadLibraryW(L"DBGHelp.dll")) };
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
		std::format(L"./pleiades/logs/Fatal_{0:%g}_{0:%h}_{0:%d}_{0:%H}_{0:%OM}_{0:%OS}.dmp", std::chrono::system_clock::now()).c_str(),
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
		   .ClientPointers = FALSE
	};

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