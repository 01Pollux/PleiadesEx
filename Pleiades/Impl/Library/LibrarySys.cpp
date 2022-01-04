#include <vector>
#include <regex>
#include <sstream>
#include <functional>
#include <filesystem>

#include <boost/system.hpp>

#if BOOST_WINDOWS
#define WIN32_MEAN_AND_LEAN
#include <boost/winapi/get_proc_address.hpp>
#include <Windows.h>
#elif BOOST_LINUX
#include <dlfcn.h>
#endif

#include <px/profiler.hpp>

#include "LibrarySys.hpp"
#include "GameData.hpp"


PX_NAMESPACE_BEGIN();

LibraryManager lib_manager;

static void PatternToBytes(const std::string& pattern, std::vector<int>& bytes)
{
	using namespace std;

	regex to_replace{ "(\\?|\\*)" };
	regex reg{ "([A-Fa-f0-9]{2})" };

	string actual = regex_replace(pattern, to_replace, "2A");
	sregex_iterator bytes_begin{ actual.begin(), actual.end(), reg }, bytes_end;

	const size_t size = distance(bytes_begin, bytes_end);
	bytes.reserve(size);

	int res;
	stringstream stream;
	stream << std::hex;

	while (bytes_begin != bytes_end)
	{
		stream << bytes_begin->str();
		stream >> res;
		stream.clear();

		bytes.push_back(res);
		bytes_begin++;
	}
}

IntPtr LibraryImpl::FindByName(const std::string& name)
{
#if BOOST_WINDOWS
	return boost::winapi::get_proc_address(HMODULE(m_ModuleHandle.get()), name.c_str());
#elif BOOST_LINUX
	return dlsym(m_ModuleHandle.get(), name.c_str());
#else
	return IntPtr::Null();
#endif
}

IntPtr LibraryImpl::FindBySignature(const std::string& signature)
{
#if BOOST_WINDOWS
	IntPtr cur_address = m_ModuleHandle;

	const PIMAGE_DOS_HEADER IDH = m_ModuleHandle.get<IMAGE_DOS_HEADER>();
	const PIMAGE_NT_HEADERS NTH = IntPtr(m_ModuleHandle + IDH->e_lfanew).get<IMAGE_NT_HEADERS>();

	const IntPtr end_address = cur_address + NTH->OptionalHeader.SizeOfImage - 1;
#elif BOOST_LINUX
	IntPtr cur_address = static_cast<void*>(m_FileHdr);
	const IntPtr end_address{ };
	auto pHDR = IntPtr(cur_address + m_FileHdr->e_phoff).get<ElfPHeader>();

	for (size_t i = 0, hdrCount = m_FileHdr->e_phnum; i < hdrCount; ++i)
	{
		ElfPHeader& HDR = pHDR[i];
		if (HDR.p_type == PT_LOAD && HDR.p_flags == (PF_X | PF_R))
		{
			end_address = PAGE_ALIGN_UP(HDR.p_filesz) + cur_address;
			break;
		}
	}
	if (end_address <= cur_address)
		return IntPtr::Null();
#endif
	std::vector<int> bytes;

	PatternToBytes(signature, bytes);

	const size_t size = bytes.size();

	bool found;
	while (cur_address < end_address)
	{
		found = true;
		for (size_t i = 0; i < size; i++)
		{
			if (bytes[i] != 0x2A && bytes[i] != cur_address.read<uint8_t>(i))
			{
				found = false;
				break;
			}
		}

		if (found)
			return cur_address;

		++cur_address;
	}

	return IntPtr::Null();
}

IntPtr LibraryImpl::FindBySignature(const ILibrary::SignaturePredicate& signature)
{
#if BOOST_WINDOWS
	MEMORY_BASIC_INFORMATION MBI{ };
	if (!VirtualQuery(m_ModuleHandle.get(), &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
		return IntPtr::Null();

	IntPtr cur_address = MBI.AllocationBase;

	const PIMAGE_DOS_HEADER IDH = cur_address.get<IMAGE_DOS_HEADER>();
	const PIMAGE_NT_HEADERS NTH = IntPtr(cur_address + IDH->e_lfanew).get<IMAGE_NT_HEADERS>();

	const IntPtr end_address = cur_address + NTH->OptionalHeader.SizeOfImage - 1;
#elif BOOST_LINUX
	IntPtr cur_address = static_cast<void*>(m_FileHdr);
	const IntPtr end_address{ };
	auto pHDR = IntPtr(cur_address + m_FileHdr->e_phoff).get<ElfPHeader>();

	for (size_t i = 0, hdrCount = m_FileHdr->e_phnum; i < hdrCount; ++i)
	{
		ElfPHeader& HDR = pHDR[i];
		if (HDR.p_type == PT_LOAD && HDR.p_flags == (PF_X | PF_R))
		{
			end_address = PAGE_ALIGN_UP(HDR.p_filesz) + cur_address;
			break;
		}
	}
	if (end_address <= cur_address)
		return IntPtr::Null();
#endif
	while (cur_address < end_address)
	{
		if (signature(cur_address, end_address))
			return cur_address;

		cur_address++;
	}

	return IntPtr::Null();
}

IntPtr LibraryImpl::FindByString(const std::string& str)
{
	if (str.empty())
		return IntPtr::Null();

	return FindBySignature(
		[str] (IntPtr cur_address, IntPtr end_address) -> bool
	{
		for (IntPtr ptr = cur_address; ptr < end_address; ptr++)
		{
			int i = 0;
			bool found = true;
			while (str[i])
			{
				if (ptr.read<char>(i) != str[i])
				{
					found = false;
					break;
				}
				++i;
			}
			if (found)
				return true;
		}

		return false;
	}
	);
}

bool LibraryManager::GoToDirectory(PlDirType type, const char* dir, char* buffer, size_t max_len)
{
	max_len -= 1;
	namespace fs = std::filesystem;

	try
	{
		auto path = fs::current_path();
		const char* actual_dir = dir ? dir : "";

		switch (type)
		{
		case PlDirType::Root:
		{
			break;
		}
		case PlDirType::Main:
		{
			path.append(LibraryManager::MainDir).append(actual_dir);
			break;
		}
		case PlDirType::Plugins:
		{
			path.append(LibraryManager::PluginsDir).append(actual_dir);
			break;
		}
		case PlDirType::Logs:
		{
			path.append(LibraryManager::LogsDir).append(actual_dir);
			break;
		}
		case PlDirType::Profiler:
		{
			path.append(LibraryManager::ProfilerDir).append(actual_dir);
			break;
		}
		case PlDirType::Sounds:
		{
			path.append(LibraryManager::SoundsDir).append(actual_dir);
			break;
		}
		default: return false;
		}

		if (!fs::exists(path))
			return false;

		auto [buf, size] = std::format_to_n(buffer, max_len, "{}", path.string());
		*buf = '\0';
		return size > 0;
	}
	catch (std::exception& ex)
	{
		*std::format_to_n(buffer, max_len, "{}", ex.what()).out = '\0';
		return false;
	}
}

void LibraryManager::BuildDirectories()
{
	namespace fs = std::filesystem;
	constexpr const char* dirs[] = {
		LibraryManager::PluginsDir,
		LibraryManager::SoundsDir,
		LibraryManager::LogsDir,
		LibraryManager::ProfilerDir,
	};

	for (auto dir : dirs)
		fs::create_directories(dir);
}

void LibraryManager::SetHostName(const std::string& name)
{
	m_HostName = name;
}

IGameData* LibraryManager::OpenGameData(IPlugin* pPlugin)
{
	return new GameData(pPlugin);
}

const std::string& LibraryManager::GetHostName()
{
	return m_HostName;
}

asmjit::JitRuntime* LibraryManager::GetRuntime()
{
	return m_Runtime.get();
}

std::string LibraryManager::GetLastError()
{
#ifdef BOOST_WINDOWS
	using namespace boost::winapi;
	DWORD err = ::GetLastError();
	LPVOID_ pStr;

	size_t size = format_message(
		FORMAT_MESSAGE_ALLOCATE_BUFFER_ | FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
		NULL,
		err,
		MAKELANGID_(LANG_NEUTRAL_, SUBLANG_DEFAULT_),
		std::bit_cast<LPSTR_>(&pStr),
		NULL,
		nullptr
	);
	if (pStr)
	{
		std::string msg{ std::bit_cast<const char*>(pStr), size };
		LocalFree(pStr);
		return msg;
	}
	else return std::format("Error Code: {:X}", err);
#elif BOOST_LINUX
	int err = errno;
	const char* str = strerror_r(err, error, maxlength);
	return str != error ? std::format("{}", str) : { };
#else
	return "GetLastError() not implemented yet.";
#endif
}

profiler::manager* LibraryManager::GetProfiler()
{
	return profiler::manager::Get();
}

PX_NAMESPACE_END();