#pragma once

#define MASS_ASSERT_STR(msg_a) msg_a
// #define MASS_ASSERT_STR(msg_a) L ## msg_a

class MAssIpcAssert
{
public:

	static void Raise(const char* file, int line, const char* func, const char* msg)
	{
	}

};