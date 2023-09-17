#pragma once

#define MASS_ASSERT_STR(msg_a) msg_a
// #define MASS_ASSERT_STR(msg_a) L ## msg_a

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

};