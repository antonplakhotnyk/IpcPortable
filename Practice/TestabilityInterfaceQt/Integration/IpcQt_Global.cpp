#include "IpcQt_Global.h"
#include <QtCore/QStringList>


static MAssIpcCall g_ipc_global({});

MAssIpcCall& Ipc::Inst()
{
	return g_ipc_global;
}

QString Ipc::GetArgParam_String(const QStringList& args, const char* arg)
{
    const int index_of_arg = args.indexOf(arg);
    if (index_of_arg < 0)
        return {};
    
	const int index = index_of_arg+1;
	if( index >= args.size() )
		return {};
	return args.at(index);
}

Ipc::Addr Ipc::GetArgServerAddr(const QStringList& args)
{
	const QString& host_name = GetArgParam_String(args, c_arg_autotest_server);
	if( host_name.isEmpty() )
		return {};

	bool ok = false;
	uint16_t target_port = GetArgParam_String(args, c_arg_autotest_server_port).toUInt(&ok);
	if( !ok )
		target_port = c_default_autotest_server_port;
	return {host_name,target_port};
}
