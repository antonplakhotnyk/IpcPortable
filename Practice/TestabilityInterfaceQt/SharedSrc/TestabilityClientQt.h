#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include "IpcQt_NetClient.h"
#include "TestabilityThreadQt.h"

class TestabilityClientQt: public QThread
{
public:

	TestabilityClientQt(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address);
	~TestabilityClientQt();

	void Start();
	bool IsConnected() const;

private:

	void Background_Main(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address);

private:


	class Internals: public QObject  
	{
	public:
		Internals(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address);
		~Internals();

		void StartConnection();

		std::atomic<bool>	m_is_connected = {false};
		IpcQt_NetClient		m_ipc_net;

	private:

		void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport);

	private:

		TestabilityGlobalQt::Addr m_connect_to_address;
	};

	IpcQt_TransthreadCaller		m_thread_caller;
	TestabilityWaitReady		m_int_ready;
	std::weak_ptr<Internals>	m_int;

	TestabilityThreadQt m_background_thread;
};

