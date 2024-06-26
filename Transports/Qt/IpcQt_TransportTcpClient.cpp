#include "IpcQt_TransportTcpClient.h"
#include "MAssIpc_Macros.h"

IpcQt_TransportTcpClient::IpcQt_TransportTcpClient(void)
:m_connection_transport{std::make_shared<IpcQt_TransporTcp>()}
{
}

IpcQt_TransportTcpClient::~IpcQt_TransportTcpClient()
{
}

void IpcQt_TransportTcpClient::StartConnection(const TestabilityGlobalQt::Addr& addr)
{
	QTcpSocket* connection = new QTcpSocket();
	m_connection_transport->AssignConnection(m_connection_transport, connection);

	connection->connectToHost(QString::fromStdString(addr.host_name), addr.target_port);
}

bool IpcQt_TransportTcpClient::WaitConnection()
{
	QTcpSocket* connection = m_connection_transport->GetConnection();
	return_x_if_equal_mass_ipc(connection, nullptr, false);
	return connection->waitForConnected();
}

std::shared_ptr<IpcQt_TransporTcp> IpcQt_TransportTcpClient::GetTransport()
{
	return m_connection_transport;
}
