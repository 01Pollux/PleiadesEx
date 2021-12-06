#pragma once

#include "SGDefines.hpp"
#include <list>
#include <ranges>
#include <iostream>

SG_NAMESPACE_BEGIN;

class IGameData;

class ITestCase
{
public:
	ITestCase(const char* name, const char* label) noexcept :
		SectionName(name), TextLabel(label)
	{
		m_Tests.push_back(this);
	}

	static void Register(IGameData*);
	static void Unregister();
	static void Render();

	virtual void OnRegister(IGameData*) abstract;
	virtual void OnUnregister() abstract;
	virtual void OnRun() abstract;

protected:
	const char* SectionName;
	const char* TextLabel;

private:
	static inline std::list<ITestCase*> m_Tests;
};

/// <summary>
/// alignof(bigstruct) == 8
/// sizeof(bigstruct) == 40
/// true_sizeof(bigstruct) == 32
/// delta = 8
/// 
/// 00000000: a0
/// 00000004: a1
/// 00000008: a2
/// 0000000C: a3
/// 00000010: a4
/// 00000014: ..
/// 00000018: a5
/// 00000020: a6
/// 00000024: ..
/// 00000028: ..
/// 
/// -----------
///	| 8 bytes |
/// |  range  |
/// -------------------------
/// | a0 | a1 | int, float	|
/// -------------------------
/// | a2 | a3 | float, float|
/// -------------------------
/// | a4 | .. | float, pad	|
/// -------------------------
/// |	a5	  | double		|
/// -------------------------
/// | a6 | .. | int, pad	|
/// -------------------------
/// 
/// in result:
/// 
/// I32
/// F32x1 (repeat 5) (4 + 1 for padding)
/// F64x1
/// I32
/// I32	(padding)
/// </summary>
struct bigstruct
{
	int a0;
	float a1, a2, a3, a4;
	double a5;
	const char* a6;
};
static_assert(sizeof(bigstruct) == 0x28);
static_assert(alignof(bigstruct) == alignof(double));
static_assert(sizeof(bigstruct) == (sizeof(int) + sizeof(float) * 5 + sizeof(double) + sizeof(int) * 2));

inline std::ostream& operator<<(std::ostream& os, const bigstruct& bs)
{
	return os << "{ "
		<< bs.a0 << ", "
		<< bs.a1 << ", "
		<< bs.a2 << ", "
		<< bs.a3 << ", "
		<< bs.a4 << ", "
		<< bs.a5 << ", "
		<< bs.a6 << " }";
}


/// <summary>
/// alignof(smallstruct) == 4
/// sizeof(smallstruct) == 8
/// true_sizeof(smallstruct) == 5
/// delta = 3
/// 
/// 00000000: a0
/// 00000001: ..
/// 00000002: ..
/// 00000003: ..
/// 00000004: a1
/// 
/// -----------
///	| 4 bytes |
/// |  range  |
/// ---------------------
/// | a0 | .. | .. | ..	|
/// ---------------------
/// |		 a1			|
/// ---------------------
/// 
/// in result:
/// 
/// I32
/// I32
/// </summary>
struct smallstruct
{
	int8_t a0;
	int a1;
};
static_assert(sizeof(smallstruct) == 0x8);
static_assert(alignof(smallstruct) == alignof(int));
static_assert(sizeof(smallstruct) == (sizeof(int8_t) * 4 + sizeof(int)));


inline std::ostream& operator<<(std::ostream& os, const smallstruct& bs)
{
	return os << "{ "
		<< bs.a0 << ", "
		<< bs.a1 << " }";
}

template<typename _Ty, size_t _Size>
inline std::ostream& operator<<(std::ostream& os, const std::array<_Ty, _Size>& arr)
{
	os << "{ ";
	for (size_t i = 0; i < _Size - 2; i++)
		os << arr[i] << ", ";
	return os << arr[_Size - 1] << " }";
}



#define SG_TEST_CLASS_BEGIN(NAME)	\
class Test_##NAME					\
{									\
public:								\
__declspec(noinline)

#define SG_TEST_CLASS_END()			}

#define SG_TEST_CALLBACK(NAME)		\
static __declspec(noinline)			\
SG::MHookRes Callback_##NAME(SG::PassRet* pRet, SG::PassArgs* pArgs, bool is_post)

#define SG_SEPARATOR	std::cout << "-------------------------------------------------------------------------------------------------------------\n"


SG_NAMESPACE_END;
