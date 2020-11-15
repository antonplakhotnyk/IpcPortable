#include "stdafx.h"
#include "IpcTcpTransportQt.h"
#include "MAssMacros.h"

IpcTcpTransportQt::IpcTcpTransportQt()
	:m_disconnect_called(false)
{
}


IpcTcpTransportQt::~IpcTcpTransportQt()
{
}

void IpcTcpTransportQt::AssignConnection(QTcpSocket* connection)
{
	m_connection = connection;

	//void error(QAbstractSocket::SocketError);

	QObject::connect(m_connection.GetP(), &QIODevice::readyRead, this, &IpcTcpTransportQt::OnReadyRead);
	QObject::connect(m_connection.GetP(), &QAbstractSocket::disconnected, this, &IpcTcpTransportQt::OnDisconnected);
	QObject::connect(m_connection.GetP(), (void(QAbstractSocket::*)(QAbstractSocket::SocketError))(&QAbstractSocket::error), this, &IpcTcpTransportQt::OnError);
	if( QAbstractSocket::ConnectedState == m_connection->state() )
		OnConnected();
	else
		QObject::connect(m_connection.GetP(), &QAbstractSocket::connected, this, &IpcTcpTransportQt::OnConnected);
}

QTcpSocket* IpcTcpTransportQt::Connection()
{
	return m_connection.GetP();
}

bool IpcTcpTransportQt::WaitRespond(size_t expected_size)
{
	m_connection->waitForReadyRead();

// 	LockInt lock(&m_wait_respound);
// 
// 	QCoreApplication::exec();
}

size_t	IpcTcpTransportQt::ReadBytesAvailable()
{
	if( !m_connection.GetP() )
		return 0;
	return m_connection->bytesAvailable();
}

void	IpcTcpTransportQt::Read(uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.GetP(), NULL);
	m_connection->read(reinterpret_cast<char*>(data), size);
}

void	IpcTcpTransportQt::Write(const uint8_t* data, size_t size)
{
	mass_return_if_equal(m_connection.GetP(), NULL);
	m_connection->write(reinterpret_cast<const char*>(data), size);
}

void IpcTcpTransportQt::OnConnected()
{
	HandlerOnConnected();
}

void IpcTcpTransportQt::OnDisconnected()
{
	if( !m_disconnect_called )
	{
		m_disconnect_called = true;
		HandlerOnDisconnected();
	}
}

void IpcTcpTransportQt::OnReadyRead()
{
	HandlerProcessTransport();
}

void IpcTcpTransportQt::OnError(QAbstractSocket::SocketError er)
{
	if( !m_disconnect_called )
	{
		m_disconnect_called = true;
		HandlerOnDisconnected();
	}
}

// void	IpcTcpTransportQt::StopWaitRespound()
// {
// // 	QCoreApplication::exit();
// }

// bool IpcTcpTransportQt::IsWaitRespound()
// {
// 	return (m_wait_respound!=0);
// }

