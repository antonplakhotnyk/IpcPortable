#pragma once

#include "MAssIpcCall.h"

#include "MAssIpc_TransthreadTarget.h"
#include "IpcQt_Serializers.h"

class Ipc
{
public:

	static constexpr uint16_t c_default_autotest_server_port = 2233;
	static constexpr char c_arg_autotest_server[] = "-autotest-server";
	static constexpr char c_arg_autotest_server_port[] = "-autotest-server-port";

	struct Addr
	{
		QString host_name;
		uint16_t target_port;

		operator bool() const
		{
			return !host_name.isEmpty();
		}
	};

	static Addr GetArgServerAddr(const QStringList& args);
	static void InitSpecificClient(const QStringList& args);
	static void SutRegister(QObject* sut_object);

	static MAssIpc_TransthreadTarget::Id CurrentThread();
	static const void* HandlersTag();


	static class MAssIpcCall& Inst();

	template<class TSpecificSut>
	static void InitClient(const QStringList& args)
	{
		if( !TSpecificSut::m_int )
			if( Ipc::Addr addr = Ipc::GetArgServerAddr(args) )
				TSpecificSut::m_int.reset(new typename TSpecificSut::Internals(addr));
	}


private:

	static QString GetArgParam_String(const QStringList& args, const char* arg);
};