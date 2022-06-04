#pragma once

#include <QtNetwork/QTcpSocket>
#include <QtCore/QPointer>
#include "MAssIpcCall.h"

class IpcTcpTransportQt: public QObject, public MAssIpcCallTransport
{
	Q_OBJECT;
public:

	IpcTcpTransportQt();
	~IpcTcpTransportQt();

	QTcpSocket* GetConnection();

signals:

	void HandlerOnDisconnected();
	void HandlerOnConnected();
	void HandlerProcessTransport();

protected:

	void	AssignConnection(QTcpSocket* connection);

// 	void StopWaitRespound();
// 	bool IsWaitRespound();

private:

	bool	WaitRespond(size_t expected_size) override;

	size_t	ReadBytesAvailable() override;
	void	Read(uint8_t* data, size_t size) override;
	void	Write(const uint8_t* data, size_t size) override;


	void OnConnected();
	void OnDisconnected();
	void OnReadyRead();
	void OnError(QAbstractSocket::SocketError er);
	void OnConnectionTimeout();

private:

	QPointer<QTcpSocket>	m_connection;

private:

// 	int						m_wait_respound;

	bool m_disconnect_called;
};

