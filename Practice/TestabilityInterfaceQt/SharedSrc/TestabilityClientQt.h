#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include "IpcClientNet.h"

class TestabilityClientQt: public QThread
{
public:

	TestabilityClientQt(const IpcClientTcpTransport::Addr& connect_to_address);
	~TestabilityClientQt();

	MAssIpcCall& Ipc();

private:

	void Background_Main(const IpcClientTcpTransport::Addr& connect_to_address);

private:

	class Internals: public QObject  
	{
	public:
		Internals(const IpcClientTcpTransport::Addr& connect_to_address);
		~Internals();

		IpcClientNet	m_ipc_net;
	};

	class WaitReady
	{
		bool m_ready = false;
		std::condition_variable	m_condition;
		std::mutex m_mutex;

	public:

		void Wait();
		void SetReady();
	};

	ThreadCallerQt m_thread_caller;
	MAssIpcCall	m_ipc_call;
	WaitReady	m_int_ready;
	std::weak_ptr<Internals> m_int;

	std::unique_ptr<QThread> m_background_thread;
};

