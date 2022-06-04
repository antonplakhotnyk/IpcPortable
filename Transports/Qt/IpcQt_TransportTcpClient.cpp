#include "IpcQt_TransportTcpClient.h"
#include "MAssIpc_Macros.h"

IpcQt_TransportTcpClient::IpcQt_TransportTcpClient(void)
{
}

IpcQt_TransportTcpClient::~IpcQt_TransportTcpClient()
{
}

void IpcQt_TransportTcpClient::StartConnection(const Addr& addr)
{
	QTcpSocket* connection = new QTcpSocket();
	AssignConnection(connection);

	connection->connectToHost(addr.host_name, addr.target_port);
}

bool IpcQt_TransportTcpClient::WaitConnection()
{
	QTcpSocket* connection = GetConnection();
	mass_return_x_if_equal(connection, nullptr, false);
	return connection->waitForConnected();
}

