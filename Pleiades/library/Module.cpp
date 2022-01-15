#include <vector>
#include <regex>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Module.hpp"

using px::IntPtr;

static std::vector<int> PatternToBytes(std::string_view pattern)
{
	using namespace std;

	regex to_replace{ "(\\?|\\*)" };
	regex reg{ "([A-Fa-f0-9]{2})" };

	string actual = regex_replace(pattern.data(), to_replace, "2A");
	sregex_iterator bytes_begin{ actual.begin(), actual.end(), reg }, bytes_end;

	const size_t size = distance(bytes_begin, bytes_end);
	std::vector<int> bytes;
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

	return bytes;
}

IntPtr LibraryImpl::FindByName(std::string_view name)
{
	if (m_IsManualMapped)
	{
		auto NTH = (GetModule() + GetModule().as<PIMAGE_DOS_HEADER>()->e_lfanew).as<PIMAGE_NT_HEADERS>();
		auto IED = (GetModule() + NTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress).as<PIMAGE_EXPORT_DIRECTORY>();

		for (DWORD i = 0; i < IED->NumberOfNames; i++)
		{
			const char* func_name = (GetModule() + (GetModule() + IED->AddressOfNames).get<DWORD>()[i]).as<char*>();

			if (std::string_view(func_name).contains(name))
			{
				WORD ordinal = (GetModule() + IED->AddressOfNames).get<WORD>()[i];
				return GetModule() + (GetModule() + IED->AddressOfFunctions).get<DWORD>()[ordinal];
			}
		}
		return nullptr;
	}
	else
	{
		return GetProcAddress(m_ModuleHandle.as<HMODULE>(), name.data());
	}
}

IntPtr LibraryImpl::FindBySignature(std::string_view signature)
{
	IntPtr cur_address = m_ModuleHandle;

	const PIMAGE_DOS_HEADER IDH = m_ModuleHandle.get<IMAGE_DOS_HEADER>();
	const PIMAGE_NT_HEADERS NTH = IntPtr(m_ModuleHandle + IDH->e_lfanew).get<IMAGE_NT_HEADERS>();

	const IntPtr end_address = cur_address + NTH->OptionalHeader.SizeOfImage - 1;

	std::vector<int> bytes = PatternToBytes(signature);

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

	return nullptr;
}

IntPtr LibraryImpl::FindBySignature(const ILibrary::SignaturePredicate& signature)
{
	IntPtr cur_address = m_ModuleHandle;

	const PIMAGE_DOS_HEADER IDH = m_ModuleHandle.get<IMAGE_DOS_HEADER>();
	const PIMAGE_NT_HEADERS NTH = IntPtr(m_ModuleHandle + IDH->e_lfanew).get<IMAGE_NT_HEADERS>();

	const IntPtr end_address = cur_address + NTH->OptionalHeader.SizeOfImage - 1;
	while (cur_address < end_address)
	{
		if (signature(cur_address, end_address))
			return cur_address;

		cur_address++;
	}

	return nullptr;
}

IntPtr LibraryImpl::FindByString(std::string_view str)
{
	if (str.empty())
		return nullptr;

	return FindBySignature(
		[str](IntPtr cur_address, IntPtr end_address) -> bool
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

LibraryImpl::~LibraryImpl()
{
	if (!m_ModuleHandle || !m_ShouldFreeModule)
		return;

	if (m_IsManualMapped)
		VirtualFree(m_ModuleHandle, 0, MEM_RELEASE);
	else FreeLibrary(static_cast<HMODULE>(m_ModuleHandle.get()));
}