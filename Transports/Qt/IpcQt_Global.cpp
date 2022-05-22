#include "IpcQt_Global.h"

static MAssIpcCall g_ipc_global({});
MAssIpcCall& IpcClient()
{
	return g_ipc_global;
}

