#include "IpcClientTcpTransport.h"
#include "MAssMacros.h"

IpcClientTcpTransport::IpcClientTcpTransport(void)
{
}

IpcClientTcpTransport::~IpcClientTcpTransport()
{
}

void IpcClientTcpTransport::Init(const Addr& addr)
{
	QTcpSocket* connection = new QTcpSocket();
	AssignConnection(connection);

	connection->connectToHost(addr.host_name, addr.target_port);
}

void IpcClientTcpTransport::WaitConnection()
{
	bool br = GetConnection()->waitForConnected();
	mass_return_if_equal(br, false);
}

