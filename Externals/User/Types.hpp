#pragma once

#include <string>
#include <windows.h>


namespace ShadowGarden
{
	using WChar = wchar_t;
	using Char = char;

#ifdef UNICODE
	using TChar = WChar;
#else
	using TChar = Char;
#endif;

	using WCString = WChar*;
	using CString = Char*;
	using TCString = TChar*;

	using CWCString = const WCString;
	using CCString = const CString;
	using CTCString = const TCString;

	using STDString = std::basic_string<TChar>;
	
	using Dword = unsigned long;
	using Word = unsigned short;
	using Byte = std::byte;
}