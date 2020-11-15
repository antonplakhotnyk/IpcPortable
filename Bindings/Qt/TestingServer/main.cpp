
#include "IpcDelegateGeneral.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include "IpcServerNet.h"
//#include "DelegateWCaller_Window.h"


void WaitProcessingEvents(uint16_t ms)
{
	QTimer timer;

	QObject::connect(&timer, &QTimer::timeout, QCoreApplication::instance(), &QCoreApplication::quit);

	timer.setSingleShot(true);
	timer.setInterval(ms);
	timer.start();
	QCoreApplication::exec();
}

static void OnClientConnected(IpcServerNet* server)
{
	check_point("");
	QCoreApplication::exit();
}

static void OnAppReady(IpcServerNet* server)
{
	QCoreApplication::exit();
}

static void Log(const std::string& msg)
{
	check_point("%s", (const char*)(msg.c_str()));
}

static void Test1()
{
	OPtr<SO<IpcServerNet> > server = new SO<IpcServerNet>;

	server->Call().AddHandler("Log", DelegateW<void(std::string msg)>().BindS(&Log));
	server->Call().AddHandler("OnAppReady", EventgateW<void()>().BindES<-1>(&OnAppReady, Del::Const<Del::Require<WPtr<SO<IpcServerNet>>>>(server)));

	{
		IpcServerNet::Handlers handlers;
		handlers.OnConnected.BindES<-1>(&OnClientConnected, Del::Const<Del::Require<WPtr<SO<IpcServerNet>>>>(server));
		handlers.OnDisconnected.BindES<-1>(&QCoreApplication::exit, Del::Const<int>(1));
		server->Init(handlers, 2277);
	}

	QCoreApplication::exec();

	auto app = server->Call();
	WaitProcessingEvents(1000);
	app["ClickPlay"]();
	WaitProcessingEvents(5000);
	app["ClickStop"]();

	QCoreApplication::exec();
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

//	DelegateWCaller_Window delegate_caller;

	Test1();

	return a.exec();
}
