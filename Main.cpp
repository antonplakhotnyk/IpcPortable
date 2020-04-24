#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "Ipc/MAssIpcCall.h"
#include <functional>

class IpcTransportMemory: public MAssIpcCallTransport
{
private:
	struct SyncData
	{
		std::mutex	lock;
		bool incoming_data = false;
		std::condition_variable cond;
		std::vector<uint8_t> data;
	};

	IpcTransportMemory(std::shared_ptr<SyncData> read, std::shared_ptr<SyncData> write)
		:m_read(read)
		, m_write(write)
	{
	}

public:

	IpcTransportMemory()
		:m_read(new SyncData)
		, m_write(new SyncData)
	{
	}

	std::shared_ptr<IpcTransportMemory> CreateComplementar()
	{
		return std::shared_ptr<IpcTransportMemory>(new IpcTransportMemory(m_write, m_read));
	}

	void WaitIncomingData()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( !m_read->incoming_data )
			m_read->cond.wait(lock);
	}

	void	WaitRespond(size_t expected_size) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( m_read->data.size()<expected_size )
			m_read->cond.wait(lock);
	}

	size_t	ReadBytesAvailable() override
	{
		std::lock_guard<std::mutex> lock(m_read->lock);
		return m_read->data.size();
	}

	void	Read(uint8_t* data, size_t size) override
	{
		std::lock_guard<std::mutex> lock(m_read->lock);
		SyncData& sync = *m_read;
		if( sync.data.size()<size )
			return;// unexpected
		std::copy(sync.data.begin(), sync.data.begin()+size, data);
		sync.data.erase(sync.data.begin(), sync.data.begin()+size);
		sync.incoming_data = false;
	}

	void	Write(const uint8_t* data, size_t size) override
	{
		std::lock_guard<std::mutex> lock(m_write->lock);
		SyncData& sync = *m_write;
		sync.data.insert(sync.data.end(), data, data+size);
		sync.incoming_data = true;
		sync.cond.notify_all();
	}

private:


	std::shared_ptr<SyncData>	m_write;
	std::shared_ptr<SyncData>	m_read;
};

//-------------------------------------------------------


static std::string Ipc_Proc1(uint8_t a, std::string b, uint32_t c)
{
// 	if( a == 1 )
// 	{
// 		(*local_data->call)("ClientProc")(a, c);
// 		return "callback";
// 	}

	return std::string("test result ")+std::to_string(a)+b+std::to_string(c);
}

static bool IsLinkUp()
{
	return false;
}

static void Ipc_Proc3(uint8_t a, std::string b, uint32_t c)
{
}

static void OnInvalidRemoteCall(MAssIpcCall::ErrorType et, std::string message)
{
}

void Main_IpcService(std::shared_ptr<IpcTransportMemory> transport_buffer)
{
	MAssIpcCall call(nullptr);

	call.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));

// 	call.AddHandler("Ipc_Proc1", DelegateW<std::string(uint8_t, std::string, uint32_t)>().BindS(&Ipc_Proc1));
 	call.AddHandler("IsLinkUp_Sta", std::function<bool()>(&IsLinkUp));
 	call.AddHandler("Ipc_Proc1", std::function<std::string(uint8_t, std::string, uint32_t)>(&Ipc_Proc1));
	call.AddHandler("Ipc_Proc3", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
//	call.AddHandler("IsLinkUp_Sta", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));


	call.SetTransport(transport_buffer);


	while( true )
	{
		call.ProcessTransport();
		transport_buffer->WaitIncomingData();
	}
}

//-------------------------------------------------------

void ClientProc(uint8_t a, uint32_t b)
{
	volatile int val = a+b;
	val = val;
}

void Main_IpcClient()
{
	MAssIpcCall call(nullptr);

// 	call.AddHandler("ClientProc", DelegateW<void(uint8_t, uint32_t)>().BindS(&ClientProc));

	call.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));

	std::shared_ptr<IpcTransportMemory> transport_buffer(new IpcTransportMemory);

	std::shared_ptr<IpcTransportMemory> complementar_buffer = transport_buffer->CreateComplementar();
	std::thread serv_thread(&Main_IpcService, complementar_buffer);

	call.SetTransport(transport_buffer);


	uint8_t a = 123;
	std::string b = "4567";
	uint32_t c = 89012345;
	bool br2 = call.WaitInvokeRet<bool>("IsLinkUp_Sta");
	std::string res2 = call.WaitInvokeRet<std::string>("Ipc_Proc1",a, b, c);
	std::string res3 = call.WaitInvokeRet<std::string>("NotExistProc", a, b, c);
	uint8_t& a1 = a;
	call.WaitInvoke("Ipc_Proc3", a1, b, c);
	call.AsyncInvoke("Ipc_Proc3", a, b, c);
	MAssIpcCall_EnumerateData enum_data = call.EnumerateRemote();
	MAssIpcCall_EnumerateData enum_data2 = call.EnumerateRemote();
	bool br = call.WaitInvokeRet<bool>("IsLinkUp_Sta");
	std::string res = call.WaitInvokeRet<std::string>("Ipc_Proc1",a, b, c);

// 	bool br = call["IsLinkUp_Sta"]();
// 	std::string res = call["Ipc_Proc1"](a, b, c);
//	res = std::string(call["Ipc_Proc1"](uint8_t(1), b, c));
	res = res;

	serv_thread.join();
}


int main()
{

	Main_IpcClient();
	return 0;
}


