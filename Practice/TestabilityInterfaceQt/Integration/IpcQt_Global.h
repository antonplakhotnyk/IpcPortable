#pragma once

#include "MAssIpcCall.h"

#include "MAssIpc_TransthreadTarget.h"
#include "IpcQt_Serializers.h"
#include "Sut.h"

class Ipc: public Sut
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

	static MAssIpc_TransthreadTarget::Id CurrentThread();
	static const void* HandlersTag();


	static class MAssIpcCall& Inst();

	template<class TSpecificSut>
	static void InitClient(const QStringList& args)
	{
		if( !TSpecificSut::s_sut_inst )
			if( Ipc::Addr addr = Ipc::GetArgServerAddr(args) )
				TSpecificSut::s_sut_inst.reset(new typename TSpecificSut::Internals(addr, Sut::GetEventHandlerMap()));
	}

private:

	static QString GetArgParam_String(const QStringList& args, const char* arg);
};