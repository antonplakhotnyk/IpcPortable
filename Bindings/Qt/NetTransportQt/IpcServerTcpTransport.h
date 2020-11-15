#pragma once

#include <QtNetwork/QTcpServer>
#include "IpcCallBindlObj.h"
#include "IpcTcpTransportQt.h"
#include "EventgateW.h"

class IpcServerTcpTransport: public IpcTcpTransportQt
{
public:

	void Init(const Handlers& handlers, uint16_t listen_port);

	IpcServerTcpTransport(void);
	~IpcServerTcpTransport();


	QTcpServer* Server();

private:

	void NewConnection();

private:

	QTcpServer	m_server;
};

