#include "TestabilityClientQt.h"
#include <QtCore/QEventLoop>

TestabilityClientQt::TestabilityClientQt(const IpcClientTcpTransport::Addr& connect_to_address)
	:m_background_thread(new BackgroundThread(this, connect_to_address))
{
	ThreadCallerQt::AddTargetThread(QThread::currentThread());
	ThreadCallerQt::AddTargetThread(m_background_thread.get());
}

TestabilityClientQt::~TestabilityClientQt()
{
	m_background_thread->terminate();
	m_background_thread->wait();
}

void TestabilityClientQt::Start()
{
	m_background_thread->start();
	m_int_ready.Wait();
}


void TestabilityClientQt::Background_Main(const IpcClientTcpTransport::Addr& connect_to_address)
{
	QEventLoop event_loop;
	std::shared_ptr<Internals> internals = std::make_shared<Internals>(connect_to_address);
	m_int = internals;
	m_int_ready.SetReady();

	internals->StartConnection();

	//	m_ipc_net.Ipc().AddHandler("AutotestServerConnected", std::function<void()>(&AutotestServerConnected));

// 	{// Connection logic should be more sofisticated
// 		while( true )
// 		{
// 			internals->m_ipc_net.GetIpcClientTcpTransport().StartConnection(connect_to_address);
// 			if( internals->m_ipc_net.GetIpcClientTcpTransport().WaitConnection() )
// 				break;
// 			QThread::sleep(5);// retry connect after 5 secconds
// 		}
// 	}

	// 	m_ipc_net.Ipc().WaitInvoke("TestProc");

//  	LOG(DETAIL_ALWAYS, STR("Tesability thread finished"));
	event_loop.exec();
}

//--------------------------------------------------

TestabilityClientQt::Internals::Internals(const IpcClientTcpTransport::Addr& connect_to_address)
	:m_connect_to_address(connect_to_address)
{
	QObject::connect(&m_ipc_net.GetIpcClientTcpTransport(), &IpcTcpTransportQt::HandlerOnDisconnected, this, &Internals::OnDisconnected, Qt::QueuedConnection);
}

TestabilityClientQt::Internals::~Internals()
{
}

void TestabilityClientQt::Internals::StartConnection()
{
	m_ipc_net.GetIpcClientTcpTransport().StartConnection(m_connect_to_address);
}

void TestabilityClientQt::Internals::OnDisconnected()
{
	StartConnection();
}

//--------------------------------------------------
void TestabilityClientQt::WaitReady::Wait()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	while( !m_ready )
		m_condition.wait(lock);
}

void TestabilityClientQt::WaitReady::SetReady()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_ready = true;
	m_condition.notify_all();
}
//--------------------------------------------------
