#include "TestabilityGlobalQt.h"
#include <QtCore/QStringList>
#include "MAssIpc_Macros.h"


static MAssIpcCall g_ipc_global({});

MAssIpcCall& TestabilityGlobalQt::Ipc()
{
	return g_ipc_global;
}

QString TestabilityGlobalQt::GetArgParam_String(const QStringList& args, const char* arg)
{
    const int index_of_arg = args.indexOf(arg);
    if (index_of_arg < 0)
        return {};
    
	const int index = index_of_arg+1;
	if( index >= args.size() )
		return {};
	return args.at(index);
}

TestabilityGlobalQt::Addr TestabilityGlobalQt::GetArgServerAddr(const QStringList& args)
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

QObject* TestabilityGlobalQt::InitCheckDeleterParent(QObject* sut_deleter_parent)
{
	if( sut_deleter_parent->thread() != QThread::currentThread() )
		sut_deleter_parent = nullptr;

	mass_assert_if_equal(sut_deleter_parent, nullptr);
	return sut_deleter_parent;
}