#include "IpcClientTcpTransport.h"
#include "MAssMacros.h"

IpcClientTcpTransport::IpcClientTcpTransport(void)
{
}

IpcClientTcpTransport::~IpcClientTcpTransport()
{
}

void IpcClientTcpTransport::StartConnection(const Addr& addr)
{
	QTcpSocket* connection = new QTcpSocket();
	AssignConnection(connection);

	connection->connectToHost(addr.host_name, addr.target_port);
}

bool IpcClientTcpTransport::WaitConnection()
{
	QTcpSocket* connection = GetConnection();
	mass_return_x_if_equal(connection, nullptr, false);
	return connection->waitForConnected();
}

