#pragma once

#include "MAssIpcCall.h"

#include "MAssIpc_TransthreadTarget.h"
#include "IpcQt_Serializers.h"

class Ipc
{
public:

	static void InitClient(const QString host_name, uint16_t target_port);
	static void SutRegister(QObject* sut_object);

	static MAssIpc_TransthreadTarget::Id CurrentThread();
	static const void* HandlersTag();


	static class MAssIpcCall& Inst();
};