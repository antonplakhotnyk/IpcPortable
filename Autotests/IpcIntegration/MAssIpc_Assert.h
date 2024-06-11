#pragma once

class MAssIpc_Assert_Autotest
{
} static constexpr assert_msg_failed_autotest;

class MAssIpc_Assert_Unexpected
{
} static constexpr assert_msg_unexpected;



class MAssIpc_Assert
{
public:

	static void Raise(volatile const char* file, int line, const char* func, const char* msg)
	{
		static bool debug_loob;
		debug_loob = true;
		while( debug_loob )
		{
			line = line;
			file = file;
			debug_loob = debug_loob;
		}

	}

	static void Raise(const char* file, int line, const char* func, const wchar_t* msg)
	{
		Raise(file, line, func, "");
	}

	static void Raise(const char* file, int line, const char* func, const decltype(assert_msg_failed_autotest)& msg)
	{
		Raise(file, line, func, "assert_msg_failed_autotest");
	}

	static void Raise(const char* file, int line, const char* func, const decltype(assert_msg_unexpected)& msg)
	{
		Raise(file, line, func, "assert_msg_unexpected");
	}

};