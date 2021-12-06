#include "Interfaces/GameData.hpp"
#include "Defines.hpp"
#include "TestCase.hpp"


SG_NAMESPACE_BEGIN;

// faster way of hooking than copy pasting
// places two prehooks and txo post hooks
#define SG_HOOK(NAME)																													\
do {																																	\
	m_HookInst->AddCallback(true, SG::HookOrder::Any, std::bind(Callback_##NAME, std::placeholders::_1, std::placeholders::_2, true));	\
	m_HookInst->AddCallback(true, SG::HookOrder::Any, std::bind(Callback_##NAME, std::placeholders::_1, std::placeholders::_2, true));	\
	m_HookInst->AddCallback(false, SG::HookOrder::Any, std::bind(Callback_##NAME, std::placeholders::_1, std::placeholders::_2, false));\
	m_HookInst->AddCallback(false, SG::HookOrder::Any, std::bind(Callback_##NAME, std::placeholders::_1, std::placeholders::_2, false));\
} while (false)


static constexpr const char* ThisCallSection = "ThisCall";

class IThisCallTest : public ITestCase
{
public:
	template<typename _Ty>
	IThisCallTest(const char* name, _Ty func_addr) :
		m_FuncAddr(std::bit_cast<void*>(func_addr)), ITestCase(ThisCallSection, name)
	{ }

	// Inherited via ITestCase
	void OnRegister(IGameData* gamedata) override
	{
		m_HookInst = SG::DetourManager->LoadHook({ }, TextLabel, gamedata, m_FuncAddr, m_FuncAddr);
		if (!m_HookInst)
			SG_LOG_MESSAGE(
				SG_MESSAGE("Failed to load hook"),
				SG_LOGARG("Name", TextLabel)
			);
	}

	void OnUnregister() final
	{
		if (m_HookInst)
			SG::DetourManager->ReleaseHook(m_HookInst);
	}

	void OnRun() override
	{ }

protected:
	SG::IHookInstance* m_HookInst;
	void* m_FuncAddr;
};



#pragma region("void Foo()")
SG_TEST_CLASS_BEGIN(Void)
void Foo()
{
	std::cout << "Calling Original 'void Foo()'\n";
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(Void)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")\n";
	return { };
}
class VoidTest : public IThisCallTest
{
public:
	VoidTest() :
		IThisCallTest("void Foo()", &Test_Void::Foo) { }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(Void);
	}

	void OnRun() override
	{
		SG_SEPARATOR;
		Test_Void test;
		test.Foo();
		SG_SEPARATOR;
	}
};
static VoidTest void_test;
#pragma endregion



#pragma region("void Foo(int)")
SG_TEST_CLASS_BEGIN(Void_int)
void Foo(int arg0)
{
	std::cout << "Calling Original 'void Foo(" << arg0 << ")'\n";
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(Void_int)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << pArgs->get<int>(0) << ")\n";
	return { };
}
class VoidIntTest : public IThisCallTest
{
public:
	VoidIntTest() :
		IThisCallTest("void Foo(int)", &Test_Void_int::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(Void);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_Void_int test;
		test.Foo(x++);
		SG_SEPARATOR;
	}
};
static VoidIntTest void_int_test;
#pragma endregion



#pragma region("int Foo(int)")
SG_TEST_CLASS_BEGIN(int_int)
int Foo(int arg0)
{
	std::cout << "Calling Original 'int Foo(" << arg0 << ")'\n";
	return arg0 + 5;
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(int_int)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		")\n\tArgs: " << pArgs->get<int>(0) << "\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<int>() << '\n';
	return { };
}
class IntIntTest : public IThisCallTest
{
public:
	IntIntTest() :
		IThisCallTest("int Foo(int)", &Test_int_int::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(int_int);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_int_int test;
		auto res = test.Foo(x++);
		std::cout << "Function returned " << res << '\n';
		SG_SEPARATOR;
	}
};
static IntIntTest int_int_test;
#pragma endregion



#pragma region("double Foo(float, int)")
SG_TEST_CLASS_BEGIN(double_float_int)
double Foo(float arg0, int arg1)
{
	std::cout << "Calling Original 'double Foo(" << arg0 << ", " << arg1 << ")'\n";
	return static_cast<double>(arg0) + static_cast<double>(arg1);
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(double_float_int)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		")\n\tArgs: " << pArgs->get<float>(0) << ", " << pArgs->get<int>(1) << "\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<double>() << '\n';
	return { };
}
class DoubleFloatIntTest : public IThisCallTest
{
public:
	DoubleFloatIntTest() :
		IThisCallTest("double Foo(float, int)", &Test_double_float_int::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(double_float_int);
	}

	void OnRun() override
	{
		static int x = 4;
		static float y = 1.6f;
		SG_SEPARATOR;
		Test_double_float_int test;
		auto res = test.Foo(y, x++);
		std::cout << "Function returned " << res << '\n';
		SG_SEPARATOR;
	}
};
static DoubleFloatIntTest double_float_int_test;
#pragma endregion



#pragma region("void Foo(double, float, int64)")
SG_TEST_CLASS_BEGIN(Void_double_float_int64)
void Foo(double arg0, float arg1, int64_t arg2)
{
	std::cout << "Calling Original 'void Foo(" << arg0 << ", " << arg1 << ", " << arg2 << ")'\n";
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(Void_double_float_int64)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		")\n\tArgs: " << pArgs->get<double>(0) << ", " << pArgs->get<float>(1) << ", " << pArgs->get<int64_t>(2) << "\n";
	return { };
}
class VoidDoubleFloatInt64Test : public IThisCallTest
{
public:
	VoidDoubleFloatInt64Test() :
		IThisCallTest("void Foo(double, float, int64)", &Test_Void_double_float_int64::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(Void_double_float_int64);
	}

	void OnRun() override
	{
		static int x = 4;
		static float y = 1.6f;
		SG_SEPARATOR;
		Test_Void_double_float_int64 test;
		test.Foo(static_cast<double>(x) + static_cast<double>(y), y, x++);
		SG_SEPARATOR;
	}
};
static VoidDoubleFloatInt64Test double_float_int64_test;
#pragma endregion



#pragma region("int32 Foo(double, float, int64)")
SG_TEST_CLASS_BEGIN(int32_double_float_int64)
int32_t Foo(double arg0, float arg1, int64_t arg2)
{
	std::cout << "Calling Original 'int Foo(" << arg0 << ", " << arg1 << ", " << arg2 << ")'\n";
	return static_cast<int32_t>(arg2);
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(int32_double_float_int64)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		")\n\tArgs: " << pArgs->get<double>(0) << ", " << pArgs->get<float>(1) << ", " << pArgs->get<int64_t>(2) << "\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<int>() << '\n';
	return { };
}
class Int32DoubleFloatInt64 : public IThisCallTest
{
public:
	Int32DoubleFloatInt64() :
		IThisCallTest("int32 Foo(double, float, int64)", &Test_int32_double_float_int64::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(int32_double_float_int64);
	}

	void OnRun() override
	{
		static int64_t x = 4;
		static float y = 1.6f;
		SG_SEPARATOR;
		Test_int32_double_float_int64 test;
		int res = test.Foo(static_cast<double>(x) + static_cast<double>(y), y, x++);
		std::cout << "Function returned " << res << '\n';
		SG_SEPARATOR;
	}
};
static Int32DoubleFloatInt64 int_double_float_int64_test;
#pragma endregion



#pragma region("float Foo(int8, int16, int32, float, double, int64)")
SG_TEST_CLASS_BEGIN(float_int8_int16_int32_float_double_int64)
float Foo(int8_t arg0, int16_t arg1, int32_t arg2, float arg3, double arg4, int64_t arg5)
{
	std::cout << "Calling Original 'void Foo(" << 
		arg0 << ", " <<
		arg1 << ", " <<
		arg2 << ", " <<
		arg3 << ", " <<
		arg4 << ", " <<
		arg5 << ")\n";
	return arg3 + 5.3f;
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(float_int8_int16_int32_float_double_int64)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		")\n\tArgs: " << 
		pArgs->get<int8_t>(0) << ", " << 
		pArgs->get<int16_t>(1) << ", " << 
		pArgs->get<int32_t>(2) << ", " << 
		pArgs->get<float>(3) << ", " << 
		pArgs->get<double>(4) << ", " << 
		pArgs->get<int64_t>(5) << "\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<float>() << '\n';
	return { };
}
class I8I16I32FDI64 : public IThisCallTest
{
public:
	I8I16I32FDI64() :
		IThisCallTest("float Foo(int8, int16, int32, float, double, int64)", &Test_float_int8_int16_int32_float_double_int64::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(float_int8_int16_int32_float_double_int64);
	}

	void OnRun() override
	{
		static int64_t x = 4;
		static float y = 1.6f;
		SG_SEPARATOR;
		Test_float_int8_int16_int32_float_double_int64 test;
		test.Foo(
			static_cast<int8_t>(x), 
			static_cast<int16_t>(x), 
			static_cast<int32_t>(x), 
			y, 
			static_cast<double>(x) + static_cast<double>(y), 
			x++
		);
		SG_SEPARATOR;
	}
};
static I8I16I32FDI64 wtf_is_this;
#pragma endregion



#pragma region("void Foo({int8, int})")
SG_TEST_CLASS_BEGIN(Void_smallstruct)
void Foo(smallstruct arg0)
{
	std::cout << "Calling Original 'void Foo(" << arg0 << ")'\n";
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(Void_smallstruct)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << pArgs->get<smallstruct>(0) << ")\n";
	return { };
}
class VoidSmallStructTest : public IThisCallTest
{
public:
	VoidSmallStructTest() :
		IThisCallTest("void Foo({int8, int})", &Test_Void_smallstruct::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(Void_smallstruct);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_Void_smallstruct test;
		test.Foo({ 'd', 90 });
		SG_SEPARATOR;
	}
};
static VoidSmallStructTest void_small_test;
#pragma endregion



#pragma region("{int8, int} Foo({int8, int})")
SG_TEST_CLASS_BEGIN(smallstruct_smallstruct)
smallstruct Foo(smallstruct arg0)
{
	std::cout << "Calling Original 'smallstruct Foo(" << arg0 << ")'\n";
	arg0.a0 = 'c';
	return arg0;
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(smallstruct_smallstruct)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << pArgs->get<smallstruct>(0) << ")\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<smallstruct>() << '\n';
	return { };
}
class SmallStructSmallStructTest : public IThisCallTest
{
public:
	SmallStructSmallStructTest() :
		IThisCallTest("{int8, int} Foo({int8, int})", &Test_smallstruct_smallstruct::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(smallstruct_smallstruct);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_smallstruct_smallstruct test;
		auto res = test.Foo({ 'd', 90 });
		std::cout << "Function returned " << res << '\n';
		SG_SEPARATOR;
	}
};
static SmallStructSmallStructTest small_small_test;
#pragma endregion



#pragma region("void Foo(bigstruct)")
SG_TEST_CLASS_BEGIN(Void_bigstruct)
void Foo(bigstruct arg0)
{
	std::cout << "Calling Original 'void Foo(" << arg0 << ")'\n";
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(bigstruct)
{
 	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << pArgs->get<bigstruct>(0) << ")\n";
	return { };
}
class VoidBigStructTest : public IThisCallTest
{
public:
	VoidBigStructTest() :
		IThisCallTest("void Foo(bigstruct)", &Test_Void_bigstruct::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(bigstruct);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_Void_bigstruct test;
		test.Foo({ 1, 2.f, 2.2f, 2.4f, 2.6f, 2.8, "Hello!" });
		SG_SEPARATOR;
	}
};
static VoidBigStructTest big_struct_test;
#pragma endregion



#pragma region("bigstruct Foo(bigstruct)")
SG_TEST_CLASS_BEGIN(bigstruct_bigstruct)
bigstruct Foo(bigstruct arg0)
{
	std::cout << "Calling Original 'bigstruct Foo(" << arg0 << ")'\n";
	arg0.a6 = "World";
	arg0.a2 = 3.3f;
	return arg0;
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(bigstruct_bigstruct)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << pArgs->get<bigstruct>(0) << ")\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<bigstruct>() << '\n';
	return { };
}
class BigStructBigStruct : public IThisCallTest
{
public:
	BigStructBigStruct() :
		IThisCallTest("bigstruct Foo(bigstruct)", &Test_bigstruct_bigstruct::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(bigstruct_bigstruct);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_bigstruct_bigstruct test;
		test.Foo({ 1, 2.f, 2.2f, 2.4f, 2.6f, 2.8, "Hello!" });
		SG_SEPARATOR;
	}
};
static BigStructBigStruct big_big_test;
#pragma endregion



#pragma region("array10 Foo(array10)")
SG_TEST_CLASS_BEGIN(array_10_array_10)
std::array<int, 10> Foo(std::array<int, 10> arg0)
{
	std::cout << "Calling Original 'array10 Foo(" << arg0 << ")'\n";
	arg0[5] = arg0[0];
	arg0[1] = arg0[2];
	arg0[0] = arg0[3];
	return arg0;
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(array_10_array_10)
{
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") <<
		"\n\tArgs: " << pArgs->get<std::array<int, 10>>(0) << ")\n";
	if (is_post)
		std::cout << "\tRet: " << pRet->get<std::array<int, 10>>() << '\n';
	return { };
}
class Array10Array10Test : public IThisCallTest
{
public:
	Array10Array10Test() :
		IThisCallTest("array10 Foo(array10)", &Test_array_10_array_10::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(array_10_array_10);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_array_10_array_10 test;
		auto arr = test.Foo({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
		std::cout << "Function returned " << arr << '\n';
		SG_SEPARATOR;
	}
};
static Array10Array10Test array10_array10_test;
#pragma endregion



#pragma region("void Foo(size_t, ...)")
SG_TEST_CLASS_BEGIN(va_args)
void Foo(size_t count, ...)
{
	std::cout << "Calling Original 'void Foo(" << count << ", ";

	va_list args;
	va_start(args, count);

	for (size_t i = 0; i < count - 1; i++)
	{
		int argx = va_arg(args, int);
		std::cout << argx << ", ";
	}
	int argx = va_arg(args, int);
	std::cout << argx << ")'\n";

	va_end(args);
}
SG_TEST_CLASS_END();
SG_TEST_CALLBACK(va_args)
{
	constexpr size_t args_offset = 0;
	constexpr size_t va_args_offset = args_offset + 1;
	size_t count = pArgs->get<size_t>(args_offset);
	std::cout << "Calling Hook (" << (is_post ? "post" : "pre") << ")" <<
		"\n\tArgs: " << count << ", ";

	if (count >= 1)
	{
		for (size_t i = 0; i < count - 1; i++)
		{
			std::cout << pArgs->get<int>(va_args_offset + i) << ", ";
		}
		std::cout << pArgs->get<int>(va_args_offset + count - 1);
	}

	std::cout << '\n';

	return { };
}
class VaArgsTest : public IThisCallTest
{
public:
	VaArgsTest() :
		IThisCallTest("void Foo(size_t, ...)", &Test_va_args::Foo)
	{ }

	// Inherited via IThisCallTest
	void OnRegister(IGameData* gamedata) override
	{
		IThisCallTest::OnRegister(gamedata);
		if (m_HookInst)
			SG_HOOK(va_args);
	}

	void OnRun() override
	{
		static int x = 4;
		SG_SEPARATOR;
		Test_va_args test;
		test.Foo(4, 3, 4, 5, 6);
		SG_SEPARATOR;
	}
};
static VaArgsTest va_args_test;
#pragma endregion


SG_NAMESPACE_END;