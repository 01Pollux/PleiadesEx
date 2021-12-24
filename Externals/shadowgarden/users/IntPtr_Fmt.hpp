#pragma once

#include "IntPtr.hpp"
#include <format>

namespace std
{
	template<class _CharT>
	struct formatter<ShadowGarden::IntPtr, _CharT>
	{
		template<typename _FmtCtx>
		auto parse(_FmtCtx& ctx)
		{
			return ctx.end();
		}

		template<typename _FmtCtx>
		auto format(ShadowGarden::IntPtr p, _FmtCtx& fc)
		{
			return std::format_to(fc.out(), "{}", p.ToPointer());
		}
	};

	template <class _CharT, class _Traits>
	basic_ostream<_CharT, _Traits>& operator<<(basic_ostream<_CharT, _Traits>& os, const ShadowGarden::IntPtr& ptr)
	{
		return os << ptr.ToPointer();
	}
}