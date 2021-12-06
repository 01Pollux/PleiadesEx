#pragma once

#include <bit>
#include <numeric>
#include <compare>

#include "../SGDefines.hpp"

SG_NAMESPACE_BEGIN;

struct IntPtr
{
public:
	using type = uint8_t*;

	_NODISCARD constexpr IntPtr(type iptr) noexcept : m_Ptr(iptr) { }
	_NODISCARD constexpr IntPtr(void* ptr) noexcept : m_Ptr(std::bit_cast<type>(ptr)) { }
	_NODISCARD constexpr IntPtr(std::nullptr_t) noexcept { }
	_NODISCARD constexpr IntPtr() = default;

	constexpr operator type() const noexcept { return m_Ptr; }
	//constexpr operator bool() const noexcept { return m_Ptr != 0; }

	template<typename _Ty = void>
	_NODISCARD _Ty* get() const noexcept { return std::bit_cast<_Ty*>(m_Ptr); }

	template<typename _Ty>
	_NODISCARD _Ty read(int offset = 0) const noexcept { return *std::bit_cast<_Ty*>(m_Ptr + offset); }

	template<typename _Ty>
	_NODISCARD _Ty& read(int offset = 0) noexcept { return *std::bit_cast<_Ty*>(m_Ptr + offset); }

	template<typename _Ty>
	void write(const _Ty& d, int offset = 0) noexcept { *std::bit_cast<_Ty*>(m_Ptr + offset) = d; }

	IntPtr& operator+=(int b) noexcept { m_Ptr += b; return *this; };
	IntPtr& operator-=(int b) noexcept { m_Ptr -= b; return *this; };

	IntPtr& operator++() noexcept { ++m_Ptr; return *this; };
	IntPtr operator++(int) noexcept { IntPtr tmp = *this; m_Ptr++; return tmp; };

	IntPtr operator--(int) noexcept { IntPtr tmp = *this; m_Ptr--; return tmp; };
	IntPtr& operator--() noexcept { --m_Ptr; return *this; };

	constexpr auto operator<=>(const IntPtr&) const noexcept = default;

	static constexpr IntPtr Null() { return { }; };

private:
	type m_Ptr{ };
};

SG_NAMESPACE_END;