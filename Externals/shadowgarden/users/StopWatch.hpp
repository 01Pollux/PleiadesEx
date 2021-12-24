#pragma once

#include <chrono>

using namespace std::chrono_literals;
namespace ShadowGarden
{
	class StopWatch
	{
	public:
		using clock_type = std::chrono::steady_clock;

		StopWatch(bool auto_start = true) noexcept
		{
			if (auto_start)
				start();
		}

		void start() noexcept
		{
			reset();
		}

		void reset() noexcept
		{
			m_StartTick = clock_type::now();
		}

		auto elapsed_nano() const noexcept
		{
			using namespace std::chrono;
			const auto delta = elapsed();
			return duration_cast<nanoseconds>(delta);
		}

		auto elapsed_micro() const noexcept
		{
			using namespace std::chrono;
			const auto delta = elapsed();
			return duration_cast<microseconds>(delta);
		}

		auto elapsed_milli() const noexcept
		{
			using namespace std::chrono;
			const auto delta = elapsed();
			return duration_cast<milliseconds>(delta);
		}

		auto elapsed_sec() const noexcept
		{
			using namespace std::chrono;
			const auto delta = elapsed();
			return duration_cast<seconds>(delta);
		}

		clock_type::duration elapsed() const noexcept
		{
			const auto now = clock_type::now();
			return now - m_StartTick;
		}

	private:
		clock_type::time_point	m_StartTick;
	};

	class CollectibleStopWatch
	{
	private:
		friend class StopWatchWrapper;
		class StopWatchWrapper
		{
		public:
			StopWatchWrapper(CollectibleStopWatch* owner) noexcept : m_Handler(owner) { }

			~StopWatchWrapper()
			{
				const auto elapsed = m_Timer.elapsed();
				m_Handler->m_Durations.emplace_back(elapsed);
			}

			StopWatchWrapper(const StopWatchWrapper&) = delete; StopWatchWrapper& operator=(const StopWatchWrapper&) = delete;
			StopWatchWrapper(StopWatchWrapper&&) = delete; StopWatchWrapper& operator=(StopWatchWrapper&&) = delete;

		private:
			StopWatch m_Timer;
			CollectibleStopWatch* m_Handler{ };
		};
		
	public:
		using clock_type = std::chrono::steady_clock;

		CollectibleStopWatch(size_t capacity = 0) { m_Durations.reserve(capacity); }

		StopWatchWrapper start() noexcept
		{
			return StopWatchWrapper{ this };
		}

		clock_type::duration avg() const noexcept
		{
			clock_type::duration avg{ };

			for (auto& x : m_Durations)
				avg += x;

			return avg / m_Durations.size();
		}

		clock_type::duration min() const noexcept
		{
			clock_type::duration min = clock_type::duration::max();

			for (auto& x : m_Durations)
				if (min > x)
					min = x;

			return min;
		}

		clock_type::duration max() const noexcept
		{
			clock_type::duration max = clock_type::duration::min();

			for (auto& x : m_Durations)
				if (max < x)
					max = x;

			return max;
		}

		void reset() noexcept { m_Durations.clear(); }

		auto& get() noexcept { return m_Durations; }
		auto& get() const noexcept { return m_Durations; }

	private:
		std::vector<clock_type::duration> m_Durations;
	};
}