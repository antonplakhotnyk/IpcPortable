#include "stdafx.h"
#include "IpcClientTcpTransport.h"

IpcClientTcpTransport::IpcClientTcpTransport(void)
{
}

IpcClientTcpTransport::~IpcClientTcpTransport()
{
}

void IpcClientTcpTransport::Init(QHostAddress address, uint16_t target_port)
{
	QTcpSocket* connection = new QTcpSocket();
	AssignConnection(connection);

	connection->connectToHost(address, target_port);
}

void IpcClientTcpTransport::WaitConnection()
{
	bool br = Connection()->waitForConnected();
	mass_return_if_equal(br, false);
}

