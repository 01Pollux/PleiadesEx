
#include <fstream>
#include <filesystem>

#include <px/profiler.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Manager.hpp"
#include "Module.hpp"

#include "plugins/GameData.hpp"


#ifdef _WIN64
#define RELOC_FLAG(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)
#define CURRENT_ARCH IMAGE_FILE_MACHINE_AMD64
#else
#define RELOC_FLAG(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define CURRENT_ARCH IMAGE_FILE_MACHINE_I386
#endif

LibraryManager::LibraryManager() : m_Runtime(std::make_unique<asmjit::JitRuntime>())
{ }

px::ILibrary* LibraryManager::ReadLibrary(std::string_view module_name)
{
	HMODULE pMod;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module_name.data(), &pMod))
		return nullptr;
	else
		return new LibraryImpl(pMod, false, false);
}

px::ILibrary* LibraryManager::OpenLibrary(std::wstring_view module_name, bool manual_map)
{
	if (!manual_map) [[likely]]
	{
		px::IntPtr pMod = LoadLibraryW(module_name.data());
		return pMod ? new LibraryImpl(pMod, true, false) : nullptr;
	}
	else
	{
		return [&]() -> px::ILibrary*
		{
			wchar_t dllPath[MAX_PATH];
			DWORD path_size = GetFullPathNameW(module_name.data(), DWORD(std::ssize(dllPath) - 1), dllPath, nullptr);
			if (!path_size)
				return nullptr;

			std::ifstream binary_file(dllPath, std::ios::binary | std::ios::ate);
			if (binary_file.fail())
				return nullptr;

			size_t file_size = static_cast<size_t>(binary_file.tellg());
			if (file_size < 0x1000)
				return nullptr;

			px::IntPtr buffer = VirtualAlloc(nullptr, file_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (!buffer)
				return nullptr;

			binary_file.seekg(0, std::ios::beg);
			binary_file.read(buffer.get<char>(), file_size);
			binary_file.close();

			auto DOS_Headers = buffer.get<IMAGE_DOS_HEADER>();
			if (DOS_Headers->e_magic != 0x5A4D)
			{
				VirtualFree(buffer, 0, MEM_RELEASE);
				return nullptr;
			}

			auto NT_Headers = (buffer + DOS_Headers->e_lfanew).get<IMAGE_NT_HEADERS>();
			auto OPT_Headers = &NT_Headers->OptionalHeader;
			auto FILE_Headers = &NT_Headers->FileHeader;

			/* Check platform */
			if (FILE_Headers->Machine != CURRENT_ARCH)
			{
				VirtualFree(buffer, 0, MEM_RELEASE);
				return nullptr;
			}

			px::IntPtr dllBuffer = VirtualAlloc(nullptr, file_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			memcpy_s(dllBuffer.get(), file_size, buffer.get(), OPT_Headers->SizeOfHeaders);

			/* Iterate sections */
			PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(NT_Headers);
			for (UINT i = 0; i != FILE_Headers->NumberOfSections; ++i, ++pSectionHeader)
			{
				if (!pSectionHeader->SizeOfRawData)
					continue;

				memcpy_s((dllBuffer + pSectionHeader->VirtualAddress).get(), pSectionHeader->SizeOfRawData, (buffer + pSectionHeader->PointerToRawData).get(), pSectionHeader->SizeOfRawData);
			}
			VirtualFree(buffer, 0, MEM_RELEASE);

			{
				PBYTE LocationDelta = (dllBuffer - OPT_Headers->ImageBase);
				if (LocationDelta)
				{
					if (!OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
					{
						VirtualFree(dllBuffer, 0, MEM_RELEASE);
						return nullptr;
					}

					PIMAGE_BASE_RELOCATION pRelocData = (dllBuffer + OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress).get<IMAGE_BASE_RELOCATION>();

					while (pRelocData->VirtualAddress)
					{
						UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
						PWORD pRelativeInfo = reinterpret_cast<PWORD>(pRelocData + 1);

						for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
						{
							if (RELOC_FLAG(*pRelativeInfo))
							{
								UINT_PTR* pPatch = (dllBuffer + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF)).get<UINT_PTR>();
								*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
							}
						}

						pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
					}
				}

				std::vector<HMODULE> loaded_modules;
				if (OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
				{
					PIMAGE_IMPORT_DESCRIPTOR pImportDescr = (dllBuffer + OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress).get<IMAGE_IMPORT_DESCRIPTOR>();

					while (pImportDescr->Name)
					{
						HMODULE hDll = LoadLibraryW((dllBuffer + pImportDescr->Name).get<wchar_t>());
						if (!hDll)
						{
							for (auto mod : loaded_modules)
								FreeLibrary(mod);
							VirtualFree(dllBuffer, 0, MEM_RELEASE);
							return nullptr;
						}
						else loaded_modules.push_back(hDll);

						PULONG_PTR pThunkRef = (dllBuffer + pImportDescr->OriginalFirstThunk).get<ULONG_PTR>();
						PULONG_PTR pFuncRef = (dllBuffer + pImportDescr->FirstThunk).get<ULONG_PTR>();

						if (!pThunkRef)
							pThunkRef = pFuncRef;

						for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
						{
							FARPROC pFunc{ };
							if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
								pFunc = GetProcAddress(hDll, reinterpret_cast<const char*>(LOWORD(*pThunkRef)));
							else
								pFunc = GetProcAddress(hDll, (dllBuffer + (*pThunkRef)).get<IMAGE_IMPORT_BY_NAME>()->Name);

							if (pFunc)
								*pFuncRef = std::bit_cast<ULONG_PTR>(pFunc);
							else
							{
								for (auto mod : loaded_modules)
									FreeLibrary(mod);
								VirtualFree(dllBuffer, 0, MEM_RELEASE);
								return nullptr;
							}
						}

						++pImportDescr;
					}
				}

				if (OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
				{
					auto pTLS = (dllBuffer + OPT_Headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress).get<IMAGE_TLS_DIRECTORY>();
					auto pCallback = std::bit_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

					for (; pCallback && *pCallback; ++pCallback)
						(*pCallback)(dllBuffer.get(), DLL_PROCESS_ATTACH, nullptr);
				}


				using DLLMainFn = BOOL(WINAPI*)(HMODULE, DWORD, PVOID);
				auto dll_main = (dllBuffer + OPT_Headers->AddressOfEntryPoint).as<DLLMainFn>();
				if (OPT_Headers->AddressOfEntryPoint && !dll_main(dllBuffer.as<HMODULE>(), DLL_PROCESS_ATTACH, nullptr))
				{
					for (auto mod : loaded_modules)
						FreeLibrary(mod);
					VirtualFree(dllBuffer, 0, MEM_RELEASE);
					return nullptr;
				}
			}
			return new LibraryImpl(dllBuffer, true, true);
		}();
	}
}

std::string LibraryManager::GoToDirectory(px::PlDirType type, std::string_view extra_path)
{
	namespace fs = std::filesystem;
	std::string path;

	auto tmp_path = fs::current_path();
	using PlDirType = px::PlDirType;

	switch (type)
	{
	case PlDirType::Root:
	{
		break;
	}
	case PlDirType::Main:
	{
		tmp_path.append(LibraryManager::MainDir).append(extra_path);
		break;
	}
	case PlDirType::Plugins:
	{
		tmp_path.append(LibraryManager::PluginsDir).append(extra_path);
		break;
	}
	case PlDirType::Logs:
	{
		tmp_path.append(LibraryManager::LogsDir).append(extra_path);
		break;
	}
	case PlDirType::Profiler:
	{
		tmp_path.append(LibraryManager::ProfilerDir).append(extra_path);
		break;
	}
	case PlDirType::Sounds:
	{
		tmp_path.append(LibraryManager::SoundsDir).append(extra_path);
		break;
	}
	[[unlikely]] default: return path;
	}

	if (fs::exists(tmp_path))
		path.assign(tmp_path.string());

	return path;
}

px::IGameData* LibraryManager::OpenGameData(px::IPlugin* pPlugin)
{
	return new GameData(pPlugin);
}

std::string_view LibraryManager::GetHostName()
{
	return m_HostName;
}

asmjit::JitRuntime* LibraryManager::GetRuntime()
{
	return m_Runtime.get();
}

std::string LibraryManager::GetLastError()
{
	return std::string();
}

px::profiler::manager* LibraryManager::GetProfiler()
{
	return px::profiler::manager::Get();
}


void LibraryManager::BuildDirectories()
{
	namespace fs = std::filesystem;
	constexpr const char* dirs[] = {
		LibraryManager::PluginsDir,
		LibraryManager::SoundsDir,
		LibraryManager::ConfigDir,
		LibraryManager::LogsDir,
		LibraryManager::ProfilerDir
	};

	for (auto dir : dirs)
		fs::create_directories(dir);
}