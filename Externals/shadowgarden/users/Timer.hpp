#pragma once

#include <chrono>

using namespace std::chrono_literals;
namespace ShadowGarden
{
	class Timer
	{
	public:
		using clock_type = std::chrono::steady_clock;

		Timer() noexcept { update(); }

		void update() noexcept
		{
			m_LastTime = clock_type::now();
		}

		bool has_elapsed(std::chrono::milliseconds ms) const noexcept
		{
			return clock_type::now() - m_LastTime >= ms;
		}

		bool update_if_elapsed(std::chrono::milliseconds ms) noexcept
		{
			if (has_elapsed(ms))
			{
				update();
				return true;
			}
			else return false;
		}

	private:
		clock_type::time_point m_LastTime;
	};

	class CountdownTimer
	{
	public:
		using clock_type = std::chrono::steady_clock;

		CountdownTimer(std::chrono::milliseconds ms) noexcept : m_EndTime(ms + clock_type::now()) { }

		void update(std::chrono::milliseconds ms) noexcept
		{
			m_EndTime = clock_type::now() + ms;
		}

		bool has_elapsed() const noexcept
		{
			return clock_type::now() - m_EndTime >= 0ms;
		}

	private:
		clock_type::time_point m_EndTime;
	};
}