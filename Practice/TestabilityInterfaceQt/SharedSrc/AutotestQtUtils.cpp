#include "AutotestQtUtils.h" 
#include "IpcQt_TransthreadCaller.h"

bool AutotestQtUtils::WaitProcessingSutReady(MAssIpcCall& sut_ipc, QEventLoop& event_loop)
{
	bool wait_success = false;
	const void* handler_tag = (void*)(&AutotestQtUtils::WaitProcessingSutReady);
	MAssIpcCall::HandlerGuard tag_remove(sut_ipc, handler_tag);

	sut_ipc.AddHandler("SutReady", std::function<void()>([&]()
	{
		wait_success = true;
		event_loop.exit();
	}), {}, IpcQt_TransthreadCaller::GetCurrentThreadId(), handler_tag);

	sut_ipc.AsyncInvoke("AutotestServerReady");
	event_loop.exec();
	return wait_success;
};

bool AutotestQtUtils::WaitIncomingCall(const char* proc_name, MAssIpcCall& sut_ipc, QEventLoop& event_loop)
{
	bool wait_success = false;
	const void* handler_tag = (void*)(&AutotestQtUtils::WaitIncomingCall);
	MAssIpcCall::HandlerGuard tag_remove(sut_ipc, handler_tag);

	sut_ipc.AddHandler(proc_name, std::function<void()>([&]()
	{
		wait_success = true;
		event_loop.exit();
	}), {}, IpcQt_TransthreadCaller::GetCurrentThreadId(), handler_tag);

	event_loop.exec();
	return wait_success;

}