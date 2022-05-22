#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include "IpcClientNet.h"

class TestabilityClientQt: public QThread
{
public:

	TestabilityClientQt(const IpcClientTcpTransport::Addr& connect_to_address);
	~TestabilityClientQt();

	void Start();

private:

	void Background_Main(const IpcClientTcpTransport::Addr& connect_to_address);

private:

	class BackgroundThread: public QThread
	{
	public:

		//	BackgroundThread must be removed and replaced by 
		//	:m_background_thread(QThread::create(&TestabilityClientQt::Background_Main, this, connect_to_address))
		//	it can not be done right now because of Qt targeting Android fails compile that string
		BackgroundThread(TestabilityClientQt* parent, const IpcClientTcpTransport::Addr& connect_to_address)
			:m_connect_to_address(connect_to_address)
			, m_parent(parent)
		{
		}

		void run() override
		{
			m_parent->Background_Main(m_connect_to_address);
		}

	private:

		IpcClientTcpTransport::Addr m_connect_to_address;
		TestabilityClientQt* m_parent;
	};

	class Internals: public QObject  
	{
	public:
		Internals(const IpcClientTcpTransport::Addr& connect_to_address);
		~Internals();

		void StartConnection();

		IpcClientNet	m_ipc_net;

	private:

		void OnDisconnected();

	private:

		IpcClientTcpTransport::Addr m_connect_to_address;
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
	WaitReady	m_int_ready;
	std::weak_ptr<Internals> m_int;

	std::unique_ptr<QThread> m_background_thread;
};

