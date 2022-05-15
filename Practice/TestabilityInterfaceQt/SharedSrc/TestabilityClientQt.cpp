#include "TestabilityClientQt.h"
#include <QtCore/QEventLoop>

TestabilityClientQt::TestabilityClientQt(const IpcClientTcpTransport::Addr& connect_to_address)
	:m_background_thread(QThread::create(&TestabilityClientQt::Background_Main, this, connect_to_address))
	,m_ipc_call({})
{
	ThreadCallerQt::AddTargetThread(QThread::currentThread());
	ThreadCallerQt::AddTargetThread(m_background_thread.get());
	m_background_thread->start();
	m_int_ready.Wait();

	if( auto internals = m_int.lock() )
		m_ipc_call = internals->m_ipc_net.Ipc();
}

TestabilityClientQt::~TestabilityClientQt()
{
	m_background_thread->terminate();
	m_background_thread->wait();
}

MAssIpcCall& TestabilityClientQt::Ipc()
{
	return m_ipc_call;
}

// static void AutotestServerConnected()
// {
// // 	QThread::sleep(10);
// 	qDebug()<<__FUNCTION__;
// 	volatile int a = 0;
// }

void TestabilityClientQt::Background_Main(const IpcClientTcpTransport::Addr& connect_to_address)
{
	QEventLoop event_loop;
	std::shared_ptr<Internals> internals = std::make_shared<Internals>(connect_to_address);
	m_int = internals;
	m_int_ready.SetReady();

	//	m_ipc_net.Ipc().AddHandler("AutotestServerConnected", std::function<void()>(&AutotestServerConnected));

	{// Connection logic should be more sofisticated
		while( true )
		{
			internals->m_ipc_net.GetIpcClientTcpTransport().StartConnection(connect_to_address);
			if( internals->m_ipc_net.GetIpcClientTcpTransport().WaitConnection() )
				break;
			QThread::sleep(5);// retry connect after 5 secconds
		}
	}

	// 	m_ipc_net.Ipc().WaitInvoke("TestProc");

//  	LOG(DETAIL_ALWAYS, STR("Tesability thread finished"));
	event_loop.exec();
}

//--------------------------------------------------

TestabilityClientQt::Internals::Internals(const IpcClientTcpTransport::Addr& connect_to_address)
{
}

TestabilityClientQt::Internals::~Internals()
{
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
