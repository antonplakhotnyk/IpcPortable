#pragma once

#include <QtNetwork/QTcpSocket>
#include <QtCore/QPointer>
#include "MAssIpcCall.h"

class IpcQt_TransporTcp: public QObject, public MAssIpc_TransportCopy
{
	Q_OBJECT;
public:

	IpcQt_TransporTcp();
	~IpcQt_TransporTcp();

	QTcpSocket* GetConnection();
	void	AssignConnection(std::weak_ptr<IpcQt_TransporTcp> transport, QTcpSocket* connection);

signals:

	void HandlerOnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport);
	void HandlerOnConnected(std::weak_ptr<IpcQt_TransporTcp> transport);
	void HandlerProcessTransport();

protected:

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

	std::weak_ptr<IpcQt_TransporTcp> m_self_transport;
	QPointer<QTcpSocket>	m_connection;

private:

// 	int						m_wait_respound;

	bool m_disconnect_called;
};


Q_DECLARE_METATYPE(std::weak_ptr<IpcQt_TransporTcp>)