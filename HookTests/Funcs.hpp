#pragma once

#include "Defines.hpp"
#include <boost/config.hpp>
#include <iostream>

SG_NAMESPACE_BEGIN;

#define SG_DECL_TEST_FN(ReturnType, NAME, ARG0T, ARG0, ARG1T, ARG1, ARG2T, ARG2, ARG3T, ARG3, RET, ...)			\
__declspec(noinline)ReturnType Return##ReturnType##_##NAME(ARG0T ARG0, ARG1T ARG1, ARG2T ARG2, ARG3T ARG3)		\
{																												\
	std::cout << "----------------------------------------------------------------------------\n";				\
	std::cout << "" << __FUNCTION__ << "(" << ARG0 << ", " << ARG1 << ", " << ARG2 << ", " << ARG3 << ")\n";	\
	if constexpr (!std::is_same_v<ReturnType, void>)															\
	{																											\
		std::cout << "Return: " << RET << '\n';																	\
		std::cout << "----------------------------------------------------------------------------\n";			\
		__VA_ARGS__;																							\
	}																											\
	std::cout << "----------------------------------------------------------------------------\n";				\
}


struct BigStruct
{
	int i{ };
	float x{ }, y{ }, z{ }, w{ };
	int32_t n{ };
	double k{ };
};

static constexpr BigStruct GetC()
{
	return BigStruct{ 1, 2.0f, 2.1f, 2.2f, 2.3f, 3, 4.5 };
}

inline std::ostream& operator<<(std::ostream& os, const BigStruct& bs)
{
	return os
		<< " { " << bs.i
		<< ", " << bs.x << ", " << bs.y << ", " << bs.z << ", " << bs.w
		<< ", " << bs.n
		<< ", " << bs.k << " } ";
}


#pragma optimize( "", off)

namespace ThisCall
{
	class Test
	{
	public:
		SG_DECL_TEST_FN(void, tcv_ifbi, int, arg0, float, arg1, bool, arg2, int, arg3, nullptr);
		SG_DECL_TEST_FN(int, tc_ifbi, int, arg0, float, arg1, bool, arg2, int, arg3, 5, return 5);
		SG_DECL_TEST_FN(int, tc_sfdl, BigStruct, arg0, float, arg1, double, arg2, int, arg3, 5, return 5);
		SG_DECL_TEST_FN(BigStruct, tc_dbdi, double, arg0, bool, arg1, double, arg2, int, arg3, GetC(), return GetC());
		SG_DECL_TEST_FN(int64_t, tc_llll, int64_t, arg0, int64_t, arg1, int64_t, arg2, int64_t, arg3, 0x123456789, return 0x123456789);
	};
}

#pragma optimize("", on)


SG_NAMESPACE_END;
