#include "IpcTcpTransportQt.h"
#include "MAssMacros.h"

IpcTcpTransportQt::IpcTcpTransportQt()
{
}


IpcTcpTransportQt::~IpcTcpTransportQt()
{
}

void IpcTcpTransportQt::AssignConnection(QTcpSocket* connection)
{
	m_connection = connection;

	//void error(QAbstractSocket::SocketError);

	QObject::connect(m_connection.data(), &QIODevice::readyRead, this, &IpcTcpTransportQt::OnReadyRead);
	QObject::connect(m_connection.data(), &QAbstractSocket::disconnected, this, &IpcTcpTransportQt::OnDisconnected);
	QObject::connect(m_connection.data(), &QAbstractSocket::errorOccurred, this, &IpcTcpTransportQt::OnError);
	if( QAbstractSocket::ConnectedState == m_connection->state() )
		OnConnected();
	else
		QObject::connect(m_connection.data(), &QAbstractSocket::connected, this, &IpcTcpTransportQt::OnConnected);
}

QTcpSocket* IpcTcpTransportQt::GetConnection()
{
	return m_connection.data();
}

bool IpcTcpTransportQt::WaitRespond(size_t expected_size)
{
	if( !m_connection )// was disconnected during wait
		return false;

	if( m_connection->bytesAvailable()>0 )
		return true;

	m_connection->waitForReadyRead();
	if( !m_connection )// was disconnected during wait
		return false;

	if( m_connection->bytesAvailable()>0 )
		return true;

	if( m_connection->state()!=QAbstractSocket::ConnectedState )
		return false;

	return false;

// 	LockInt lock(&m_wait_respound);
// 
// 	QCoreApplication::exec();
}

size_t	IpcTcpTransportQt::ReadBytesAvailable()
{
	if( !m_connection.data() )
		return 0;
	return m_connection->bytesAvailable();
}

void	IpcTcpTransportQt::Read(uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.data(), nullptr);
	m_connection->read(reinterpret_cast<char*>(data), size);
}

void	IpcTcpTransportQt::Write(const uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.data(), nullptr);
	auto ir = m_connection->write(reinterpret_cast<const char*>(data), size);
	mass_return_if_not_equal(ir, size);
	bool br = m_connection->waitForBytesWritten();
	mass_return_if_not_equal(br, true);
}

void IpcTcpTransportQt::OnConnected()
{
	HandlerOnConnected();
}

void IpcTcpTransportQt::OnDisconnected()
{
	if( m_connection )
	{
		m_connection.clear();
		HandlerOnDisconnected();
	}
}

void IpcTcpTransportQt::OnReadyRead()
{
	HandlerProcessTransport();
}

void IpcTcpTransportQt::OnError(QAbstractSocket::SocketError er)
{
	if(er!=QAbstractSocket::SocketTimeoutError)
		OnDisconnected();
}

// void	IpcTcpTransportQt::StopWaitRespound()
// {
// // 	QCoreApplication::exit();
// }

// bool IpcTcpTransportQt::IsWaitRespound()
// {
// 	return (m_wait_respound!=0);
// }

