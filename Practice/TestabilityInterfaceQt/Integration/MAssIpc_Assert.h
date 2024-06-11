#pragma once


class MAssIpcAssertMsg_Autotest
{
} static constexpr failed_autotest;

class MAssIpcAssertMsg_Unexpected
{
} static constexpr assert_msg_unexpected;

class MAssIpc_Assert
{
public:

	static void Raise(const char* file, int line, const char* func, const wchar_t* msg);
	static void Raise(const char* file, int line, const char* func, const char* msg);
	static void Raise(const char* file, int line, const char* func, const MAssIpcAssertMsg_Autotest& msg);
	static void Raise(const char* file, int line, const char* func, const MAssIpcAssertMsg_Unexpected& mass_assert_msg_unexpected);
};