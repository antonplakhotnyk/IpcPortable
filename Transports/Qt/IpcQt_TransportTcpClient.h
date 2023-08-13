#pragma once

#include "MAssIpcCall.h"
#include <QtCore/QDataStream>
#include <QtNetwork/QHostAddress>
#include "IpcQt_TransporTcp.h"
#include "TestabilityGlobalQt.h"


class IpcQt_TransportTcpClient: public QObject
{
public:

	IpcQt_TransportTcpClient(void);
	~IpcQt_TransportTcpClient();

	void StartConnection(const TestabilityGlobalQt::Addr& addr);
	bool WaitConnection();

	std::shared_ptr<IpcQt_TransporTcp> GetTransport();

private:

	std::shared_ptr<IpcQt_TransporTcp> m_connection_transport;
};


