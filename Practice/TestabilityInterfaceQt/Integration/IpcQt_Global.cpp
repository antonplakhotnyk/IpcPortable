#include "IpcQt_Global.h"

static MAssIpcCall g_ipc_global({});
MAssIpcCall& Ipc::Inst()
{
	return g_ipc_global;
}

