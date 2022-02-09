#pragma once

#include <px/defines.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "msdetour/detours.h"

namespace detour_detail
{

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

		LONG attach() noexcept
		{
			LONG err = ERROR_SET_OR_UNSET_TOO_LATE;

			if (!m_IsSet)
			{
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

		[[nodiscard]] bool is_set() const noexcept { return m_IsSet; }

		[[nodiscard]] void* original_function() const noexcept { return m_ActualFunc; }

		[[nodiscard]] void* callback_function() const noexcept { return m_Callback; }

		~Detour() noexcept(false)
		{
			detach();
		}

		[[nodiscard]] bool operator==(const Detour& o) const noexcept { return m_Callback == o.m_Callback; }

	public:
		Detour() = default;
		Detour(const Detour&) = delete;  Detour& operator=(const Detour&) = delete;
		Detour(Detour&&) = default; Detour& operator=(Detour&&) = default;

		[[nodiscard]] static constexpr size_t offset_to_m_ActualFunc() noexcept
		{
			return offsetof(Detour, m_ActualFunc);
		}

	private:
		address_type m_Callback{ };
		address_type m_ActualFunc{ };
		bool  m_IsSet = false;
	};
}
