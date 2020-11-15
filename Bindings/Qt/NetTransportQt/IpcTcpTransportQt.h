#pragma once

#include <QtNetwork/QTcpSocket>
#include <QtCore/QPointer>
#include "IpcCall.h"
#include "EventgateW.h"
#include "ScopedPtrQtSafe.h"

class IpcTcpTransportQt: public QObject, public IpcCallTransport
{
public:

	struct Handlers
	{
		EventgateW<void()> OnDisconnected;
		EventgateW<void()> OnConnected;
		EventgateW<void()> ProcessTransport;
	};

	IpcTcpTransportQt();
	~IpcTcpTransportQt();

	void Init(const Handlers& handlers);
	QTcpSocket* Connection();

protected:

	void	AssignConnection(QTcpSocket* connection);

// 	void StopWaitRespound();
// 	bool IsWaitRespound();

private:

	void	WaitRespound() override;

	size_t	ReadBytesAvailable() override;
	void	Read(uint8_t* data, size_t size) override;
	void	Write(const uint8_t* data, size_t size) override;


	void OnConnected();
	void OnDisconnected();
	void OnReadyRead();
	void OnError(QAbstractSocket::SocketError er);

private:

	ScopedPtrQtSafe<QTcpSocket>	m_connection;

private:

// 	int						m_wait_respound;

	bool m_disconnect_called;
	Handlers m_handlers;
};

