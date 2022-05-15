#include <QtWidgets/QApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMainWindow>
#include <QtCore/QDebug>
#include "ApplicationWithEvents.h"

#include <iostream>

#include "IpcServerNet.h"
#include "IpcDataStreamSerializersQt.h"
#include "AutotestServer.h"


//-------------------------------------------------------

//-------------------------------------------------------
//-------------------------------------------------------
//-------------------------------------------------------

class Autotest1: public AutotestBase
{
public:
	Autotest1(MAssIpcCall& sut_ipc)
		:AutotestBase(sut_ipc)
	{
// 		m_sut_ipc.AddHandler("TestProc", std::function<void()>(std::bind(&Autotest1::TestProc, this)), {}, ThreadCallerQt::GetCurrentThreadId(), Tag());
	}

	void TestProc()
	{
		std::cout<<__FUNCTION__<<std::endl;
	}

	void Run() override
	{
		QEventLoop event_loop;

		bool br;

		br = m_sut_ipc.WaitInvokeRet<bool>("OpenFile", QByteArray{"Test-data"});
		br = m_sut_ipc.WaitInvokeRet<bool>("OpenFile", QByteArray{"return false"});

		QString str = m_sut_ipc.WaitInvokeRet<QString>("TransfetString", QString{" string data"});

		event_loop.exec();
	}

};


class AutotestContainer1
{
public:

	std::unique_ptr<AutotestBase> Create(MAssIpcCall& sut_ipc)
	{
		size_t create_index = m_index;
		m_index++;
		switch( create_index )
		{
			case 0: return std::unique_ptr<AutotestBase>{new Autotest1(sut_ipc)};
			default: return {};
		}
	}

private:

	size_t m_index = 0;
};
//-------------------------------------------------------

// class AutotestContainer1: public AutotestContainer
// {
// public:
// 
// 	AutotestContainer1()
// 	{
// 		m_creators.push_back([](MAssIpcCall& sut_ipc){new Autotest1(sut_ipc)});
// 	}
// 
// 	size_t			GetIndexCount() override
// 	{
// 		return m_creators.size();
// 	}
// 
// 	std::unique_ptr<AutotestBase> Create(size_t index, MAssIpcCall& sut_ipc) override
// 	{
// 		return_x_if_equal(index>=m_creators.size(), true, nullptr);
// 		return m_creators[i](sut_ipc);
// 	}
// 
// private:
// 
// 	std::vector<TAutotestCreate> m_creators;
// };

//-------------------------------------------------------

// AutotestBase* AutotestContainer1(MAssIpcCall& sut_ipc)
// {
// 	return new Autotest1(sut_ipc);
// }

//-------------------------------------------------------
// class Thread1: public QThread
// {
// public:
// 	Thread1()
// 	{
// // 		start();
// 	}
// 
// 	~Thread1()
// 	{
// 		exit();
// 		wait();
// 	}
// 
// 	void run()
// 	{
// 		exec();
// 	}
// 
// };
// 
// 
// void TestThread()
// {
// 	Thread1 thread1;
// 	CallReceiver* receiver1 = new CallReceiver(nullptr);
// 	receiver1->moveToThread(&thread1);
// 
// // 	QObject::connect(&thread1, &QThread::started, receiver1, &CallReceiver::Slot);
// 	QObject::connect(receiver1, &CallReceiver::Signal, receiver1, &CallReceiver::Slot, Qt::BlockingQueuedConnection);
// 
// 	thread1.start();
// 
// 	receiver1->Signal();
// 
// 	CallEvent call_event;
// //	thread1.postEvent(receiver1, &call_event);
// 	volatile int a = 0;
// }
// 
//-------------------------------------------------------


int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setAutoSipEnabled(false);

// 	TestThread();


// 	AutotestServer autotest_server({&AutotestContainer1});
	AutotestServer::Params params;
	params.autotest_container = std::bind(&AutotestContainer1::Create, std::shared_ptr<AutotestContainer1>(new AutotestContainer1), std::placeholders::_1);
	AutotestServer autotest_server(params);

// 	std::cout<<"Test"<<std::endl;

	QApplication::exec();

	return 0;
}
