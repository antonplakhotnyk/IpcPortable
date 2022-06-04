#include "IpcQt_TransporTcp.h"
#include "MAssIpc_Macros.h"
#include <QtCore/QTimer>

IpcQt_TransporTcp::IpcQt_TransporTcp()
{
}


IpcQt_TransporTcp::~IpcQt_TransporTcp()
{
}

void IpcQt_TransporTcp::AssignConnection(QTcpSocket* connection)
{
	if( m_connection )
		m_connection->deleteLater();
	m_connection = connection;

	//void error(QAbstractSocket::SocketError);

	QObject::connect(m_connection.data(), &QIODevice::readyRead, this, &IpcQt_TransporTcp::OnReadyRead);
	QObject::connect(m_connection.data(), &QAbstractSocket::disconnected, this, &IpcQt_TransporTcp::OnDisconnected);
	QObject::connect(m_connection.data(), &QAbstractSocket::errorOccurred, this, &IpcQt_TransporTcp::OnError);
	if( QAbstractSocket::ConnectedState == m_connection->state() )
		OnConnected();
	else
	{
		QTimer::singleShot(std::chrono::milliseconds(10000), this, &IpcQt_TransporTcp::OnConnectionTimeout);
		QObject::connect(m_connection.data(), &QAbstractSocket::connected, this, &IpcQt_TransporTcp::OnConnected);
	}
}

QTcpSocket* IpcQt_TransporTcp::GetConnection()
{
	return m_connection.data();
}

bool IpcQt_TransporTcp::WaitRespond(size_t expected_size)
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

	return true;

// 	LockInt lock(&m_wait_respound);
// 
// 	QCoreApplication::exec();
}

size_t	IpcQt_TransporTcp::ReadBytesAvailable()
{
	if( !m_connection.data() )
		return 0;
	return m_connection->bytesAvailable();
}

void	IpcQt_TransporTcp::Read(uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.data(), nullptr);
	m_connection->read(reinterpret_cast<char*>(data), size);
}

void	IpcQt_TransporTcp::Write(const uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.data(), nullptr);
	mass_return_if_not_equal(m_connection->state(), QAbstractSocket::ConnectedState);
	auto ir = m_connection->write(reinterpret_cast<const char*>(data), size);
	mass_return_if_not_equal(ir, size);
	bool br = m_connection->waitForBytesWritten();
	mass_return_if_not_equal(br, true);
}

void IpcQt_TransporTcp::OnConnected()
{
	HandlerOnConnected();
}

void IpcQt_TransporTcp::OnDisconnected()
{
	if( m_connection )
	{
		m_connection->deleteLater();
		m_connection.clear();
		HandlerOnDisconnected();
	}
}

void IpcQt_TransporTcp::OnReadyRead()
{
	HandlerProcessTransport();
}

void IpcQt_TransporTcp::OnError(QAbstractSocket::SocketError er)
{
	if(er!=QAbstractSocket::SocketTimeoutError)
		OnDisconnected();
}

void IpcQt_TransporTcp::OnConnectionTimeout()
{
	if(m_connection)
		if( m_connection->state()!=QAbstractSocket::ConnectedState )
			OnDisconnected();
}

// void	IpcQt_TransporTcp::StopWaitRespound()
// {
// // 	QCoreApplication::exit();
// }

// bool IpcQt_TransporTcp::IsWaitRespound()
// {
// 	return (m_wait_respound!=0);
// }
