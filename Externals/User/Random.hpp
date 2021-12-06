#pragma once

#include <random>

namespace ShadowGarden
{
	namespace Random
	{
		using default_device = std::random_device;

		template<typename _Eng = default_device>
		_NODISCARD inline bool Bool(_Eng& seed)
		{
			const std::bernoulli_distribution r(0.5);
			return r(seed);
		}

		_NODISCARD inline bool Bool()
		{
			default_device device;
			std::mt19937 eng{ device() };
			return Bool(eng);
		}

#define SG_RANDOM_INT_FN(NAME, TYPE)									\
		template<typename _Eng = default_device>						\
		_NODISCARD inline TYPE NAME(TYPE min, TYPE max, _Eng& seed)		\
		{																\
			const std::uniform_int_distribution<TYPE> r(min, max);		\
			return r(seed);												\
		}																\
																		\
		_NODISCARD inline TYPE NAME(TYPE min, TYPE max)					\
		{																\
			default_device device;										\
			std::mt19937 eng{ device() };								\
			return NAME(min, max, eng);									\
		}																
		
		SG_RANDOM_INT_FN(Int8, int16_t);
		SG_RANDOM_INT_FN(Int16, int16_t);
		SG_RANDOM_INT_FN(Int32, int32_t);
		SG_RANDOM_INT_FN(Int64, int64_t);

		SG_RANDOM_INT_FN(UInt8, int16_t);
		SG_RANDOM_INT_FN(UInt16, uint16_t);
		SG_RANDOM_INT_FN(UInt32, uint32_t);
		SG_RANDOM_INT_FN(UInt64, uint64_t);

		template<typename _Eng = default_device>
		_NODISCARD inline float Float(float min, float max, _Eng& seed)
		{
			const std::uniform_real_distribution<float> r(min, max);
			return r(seed);
		}

		_NODISCARD inline float Float(float min, float max)
		{
			default_device device;
			std::mt19937 eng{ device() };
			return Float(min, max, eng);
		}

		template<typename _Eng = default_device>
		_NODISCARD inline double Double(double min, double max, _Eng& seed)
		{
			const std::uniform_real_distribution<double> r(min, max);
			return r(seed);
		}

		_NODISCARD inline double Double(double min, double max)
		{
			default_device device;
			std::mt19937 eng{ device() };
			return Double(min, max, eng);
		}
	}
}