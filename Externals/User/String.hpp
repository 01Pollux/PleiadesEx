#pragma once

#include <xutility>
#include <algorithm>
#include <chrono>

using namespace std::string_view_literals;

namespace ShadowGarden
{
	template<typename _ToTy, typename _FromTy>
	_NODISCARD constexpr _ToTy StringTransform(const _FromTy& str) noexcept
	{
		// same type
		if constexpr (std::is_same_v<_ToTy, _FromTy>)
			return str;
		// from const _char* to _string_view
		else if constexpr (std::is_pointer_v<_FromTy>)
			return StringTransform<_ToTy>(std::basic_string_view<std::remove_pointer_t<_FromTy>>(str));
		else
		{
			if (str.empty())
				return { };
			else
			{
				// from bigger type to smaller type, eg: wstring to string, u32string to u8string, etc...
				if constexpr (sizeof(_FromTy::value_type) > sizeof(_ToTy::value_type))
				{
					_ToTy buf(str.size(), 0);
					std::transform(std::begin(str), std::end(str), std::begin(buf), [] (const _FromTy::value_type c) { return static_cast<_ToTy::value_type>(c); });
					return buf;
				}
				// from smaller type to bigger type, eg: string to wstring, etc...
				else return { std::begin(str), std::end(str) };
			}
		}
	}

	// support for array of characters
	template<typename _ToTy, typename _FromTy, size_t _Size>
	_NODISCARD constexpr _ToTy StringTransform(const _FromTy(&str)[_Size]) noexcept
	{
		if constexpr (std::is_same_v<_ToTy::value_type, _FromTy>)
			return str;

		else if constexpr (_Size < 1)
			return { };
		else return { std::begin(str), std::end(str) };
	}

	template<typename _CharTy>
	_NODISCARD inline decltype(auto) FormatTime(const std::basic_string_view<_CharTy>& fmt)
	{
		return std::format(fmt, std::chrono::system_clock::now());
	}

	template<typename _CharTy, size_t _Size>
	_NODISCARD inline decltype(auto) FormatTime(const _CharTy (&fmt)[_Size])
	{
		return std::format(fmt, std::chrono::system_clock::now());
	}
}