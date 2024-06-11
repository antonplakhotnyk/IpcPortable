#pragma once


// It is not implementation it is example or template which must be implemented during integration with users code base
class MAssIpc_Assert
{
public:

	// next functions must be able to take any msg parameter passed through return_XXXX_msg or assert_XXXX_msg macros
	// may be more then one overloaded functions with different type of msg
	static void Raise(const char* file, int line, const char* func, const wchar_t* msg);
	static void Raise(const char* file, int line, const char* func, const char* msg);
	static void Raise(const char* file, int line, const char* func, const decltype(assert_msg_failed_autotest)& msg);
	static void Raise(const char* file, int line, const char* func, const decltype(assert_msg_unexpected)& msg);
};