#pragma once

// #define MASS_ASSERT_STR(msg_a) L ## msg_a
#define MASS_ASSERT_STR(msg_a) msg_a

class MAssIpc_AutotestMsg
{
} static constexpr failed_autotest;

class MAssIpc_Assert
{
public:

	static void Raise(const char* file, int line, const char* func, const wchar_t* msg);
	static void Raise(const char* file, int line, const char* func, const char* msg);
	static void Raise(const char* file, int line, const char* func, const MAssIpc_AutotestMsg& msg);
};