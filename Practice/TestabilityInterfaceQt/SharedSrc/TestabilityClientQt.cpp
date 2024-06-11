#include "TestabilityClientQt.h"
#include <QtCore/QEventLoop>

TestabilityClientQt::TestabilityClientQt(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address)
	:m_background_thread(nullptr, [this, &ipc_connection, connect_to_address](){Background_Main(ipc_connection, connect_to_address);})
{
	IpcQt_TransthreadCaller::AddTargetThread(QThread::currentThread());
	IpcQt_TransthreadCaller::AddTargetThread(&m_background_thread);
}

TestabilityClientQt::~TestabilityClientQt()
{
}

void TestabilityClientQt::Start()
{
	m_background_thread.start();
	m_int_ready.Wait();
}

bool TestabilityClientQt::IsConnected() const
{
	auto internals = m_int.lock();
	return internals->m_is_connected;
}

void TestabilityClientQt::Background_Main(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address)
{
	QEventLoop event_loop;
	std::shared_ptr<Internals> internals = std::make_shared<Internals>(ipc_connection, connect_to_address);
	m_int = internals;
	m_int_ready.SetReady();

	internals->StartConnection();

	event_loop.exec();
}

//--------------------------------------------------

TestabilityClientQt::Internals::Internals(MAssIpcCall& ipc_connection, const TestabilityGlobalQt::Addr& connect_to_address)
	:m_connect_to_address(connect_to_address)
	, m_ipc_net(ipc_connection)
{
	QObject::connect(m_ipc_net.GetIpcClientTcpTransport()->GetTransport().get(), &IpcQt_TransporTcp::HandlerOnConnected, this, [this]()
	{
		m_is_connected = true;
	}, Qt::DirectConnection);
	QObject::connect(m_ipc_net.GetIpcClientTcpTransport()->GetTransport().get(), &IpcQt_TransporTcp::HandlerOnDisconnected, this, [this]()
	{
		m_is_connected = false;
	}, Qt::DirectConnection);

	QObject::connect(m_ipc_net.GetIpcClientTcpTransport()->GetTransport().get(), &IpcQt_TransporTcp::HandlerOnDisconnected, this, &Internals::OnDisconnected, Qt::QueuedConnection);
}

TestabilityClientQt::Internals::~Internals()
{
}

void TestabilityClientQt::Internals::StartConnection()
{
	m_ipc_net.GetIpcClientTcpTransport()->StartConnection(m_connect_to_address);
}

void TestabilityClientQt::Internals::OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport)
{
	StartConnection();
}

