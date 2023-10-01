#pragma once

// uncomment one of next lines, MASS_ASSERT_STR used in mass_return_XXXX macros to create text message
#define MASS_ASSERT_STR(msg_a) msg_a
// #define MASS_ASSERT_STR(msg_a) L ## msg_a

class MAssIpc_Assert
{
public:

	// next functions must be able to take msg created by MASS_ASSERT_STR
	// may be more then one overloaded functions with different type of msg which passed through mass_return_XXXX_msg macros
	static void Raise(const char* file, int line, const char* func, const char* msg)
	{
	}
// 	static void Raise(const char* file, int line, const char* func, const wchar_t* msg)
// 	{
// 	}

};