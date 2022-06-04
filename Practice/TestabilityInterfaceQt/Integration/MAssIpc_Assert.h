#pragma once

#define MASS_ASSERT_STR(msg_a) L ## msg_a

class MAssIpc_Assert
{
public:

	static void Raise(const char* file, int line, const char* func, const wchar_t* msg);
	static void Raise(const char* file, int line, const char* func, const char* msg);
};