#pragma once

#include <boost/system.hpp>
#include "SGDefines.hpp"

#if BOOST_WINDOWS
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include "MSDetour/detours.hpp"


SG_NAMESPACE_BEGIN;

class Detour
{
public:
	using address_type = void*;
	static constexpr LONG ERROR_SET_OR_UNSET_TOO_LATE = 0x4860139;

	Detour(address_type addr, address_type callback)
	{
		attach(addr, callback);
	}

	LONG attach(address_type addr, address_type callback) noexcept
	{
		LONG err = ERROR_SET_OR_UNSET_TOO_LATE;

		if (!m_IsSet)
		{
			this->m_Callback = callback;
			this->m_ActualFunc = addr;

			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&m_ActualFunc, m_Callback);

			err = DetourTransactionCommit();
			m_IsSet = err == ERROR_SUCCESS;
		}

		return err;
	}

	LONG detach() noexcept
	{
		LONG err = ERROR_SET_OR_UNSET_TOO_LATE;

		if (m_IsSet)
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&m_ActualFunc, m_Callback);
			err = DetourTransactionCommit();
			m_IsSet = err != ERROR_SUCCESS;
		}

		return err;
	}

	bool is_set() const noexcept { return m_IsSet; }

	void* original_function() const noexcept { return m_ActualFunc; }

	void* callback_function() const noexcept { return m_Callback; }

	~Detour() noexcept(false)
	{
		detach();
	}

	bool operator==(const Detour& o) const noexcept { return m_Callback == o.m_Callback; }

public:
	Detour() = default;
	Detour(const Detour&) = delete;  Detour& operator=(const Detour&) = delete;
	Detour(Detour&&) = default; Detour& operator=(Detour&&) = default;

private:
	address_type m_Callback{ };
	address_type m_ActualFunc{ };
	bool  m_IsSet = false;
};

SG_NAMESPACE_END;

#else
#pragma error("Detour class not implemenetd outside of Window's platform")
#endif