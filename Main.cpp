#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "Ipc/MAssIpcCall.h"
#include <functional>
#include "Ipc/MAssMacros.h"
#include <sstream>

// struct TestStruct
// {
// };
// 
// template<>
// struct MAssIpcType<TestStruct>									
// {																
// static constexpr const char name_value[sizeof("test")] = "test";	
// inline static constexpr const char* NameValue()					
// {																
// return "test";
// }																
// };


class IpcTransportMemory: public MAssIpcCallTransport
{
private:
	struct SyncData
	{
		std::mutex	lock;
		bool incoming_data = false;
		bool cancel_wait_respond = false;
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

	bool	WaitRespond(size_t expected_size) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( (m_read->data.size()<expected_size) && (!m_read->cancel_wait_respond) )
			m_read->cond.wait(lock);
		return !m_read->cancel_wait_respond;
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

	void CancelWaitRespond()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		SyncData& sync = *m_read;
		sync.cancel_wait_respond = true;
		sync.cond.notify_all();
	}

private:


	std::shared_ptr<SyncData>	m_write;
	std::shared_ptr<SyncData>	m_read;
};

//-------------------------------------------------------

class IpcPackerTransportMemory: public MAssIpcPacketTransport
{
private:
	struct SyncData
	{
		std::mutex	lock;
		bool incoming_data = false;
		bool cancel_wait_respond = false;
		std::condition_variable cond;
		std::list<std::unique_ptr<MAssIpcData> > data;
	};

	class MemoryPacket: public MAssIpcData
	{
	public:

		MemoryPacket(size_t size)
			:m_data(size)
		{
		}

	private:

		size_t Size() const override
		{
			return m_data.size();
		}

		uint8_t* Data() override
		{
			return m_data.data();
		}

	private:

		std::vector<uint8_t>	m_data;
	};

	IpcPackerTransportMemory(std::shared_ptr<SyncData> read, std::shared_ptr<SyncData> write)
		:m_read(read)
		, m_write(write)
	{
	}

public:

	IpcPackerTransportMemory()
		:m_read(new SyncData)
		, m_write(new SyncData)
	{
	}

	std::shared_ptr<IpcPackerTransportMemory> CreateComplementar()
	{
		return std::shared_ptr<IpcPackerTransportMemory>(new IpcPackerTransportMemory(m_write, m_read));
	}

	void WaitIncomingData()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( !m_read->incoming_data )
			m_read->cond.wait(lock);
	}

	std::unique_ptr<MAssIpcData> Create(size_t size) override
	{
		return std::unique_ptr<MAssIpcData>(new MemoryPacket(size));
	}

	bool	Read(bool wait_incoming_packet, std::unique_ptr<MAssIpcData>* packet) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( m_read->data.empty() && (!m_read->cancel_wait_respond) )
		{
			if( wait_incoming_packet )
				m_read->cond.wait(lock);
			else
				return true;
		}
		if( m_read->cancel_wait_respond )
			return false;

		*packet = std::move(m_read->data.front());
		m_read->data.pop_front();
		return true;
	}

	void	Write(std::unique_ptr<MAssIpcData> packet) override
	{
		std::lock_guard<std::mutex> lock(m_write->lock);
		SyncData& sync = *m_write;
		sync.data.push_back(std::move(packet));
		sync.incoming_data = true;
		sync.cond.notify_all();
	}

	void CancelWaitRespond()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		SyncData& sync = *m_read;
		sync.cancel_wait_respond = true;
		sync.cond.notify_all();
	}

private:


	std::shared_ptr<SyncData>	m_write;
	std::shared_ptr<SyncData>	m_read;
};

//-------------------------------------------------------
class MAssCallThreadTransport_Stub: public MAssCallThreadTransport
{
public:

	void			CallFromThread(MAssThread::Id thread_id, const std::shared_ptr<Job>& job) override
	{
		job->Invoke();
	}

	MAssThread::Id	GetResultSendThreadId() override
	{
		return MAssThread::Id(2);
	}
};

//-------------------------------------------------------

static void OnInvalidRemoteCall(MAssIpcCall::ErrorType et, std::string message)
{
}


//-------------------------------------------------------

static bool s_test_threads_run = true;

static std::string TestThreads_HandlerWithCallBack(MAssIpcCall call)
{
	std::string res = call.WaitInvokeRet<std::string>("TestThreads_SenderCallBack");
	return res;
}

static std::string TestThreads_SenderCallBack(MAssIpcCall call)
{
	return "TestThreads_SenderCallBack";
}

static std::string TestThreads_Proc3(uint8_t a, std::string b, uint32_t c)
{
	std::stringstream ss;
	ss<<a<<b<<c;

	return ss.str();
}

void TestThreads_Handler(std::shared_ptr<IpcPackerTransportMemory> transport_buffer, MAssIpcCall call)
{
	while( true )
	{
		call.ProcessTransport();
		if( !s_test_threads_run )
			break;
// 		transport_buffer->WaitIncomingData();
	}
}

void TestThreads_Sender(std::shared_ptr<IpcPackerTransportMemory> transport_buffer, MAssIpcCall call)
{
	std::stringstream ss;
	ss<<std::this_thread::get_id();

	uint8_t a = 1;
	std::string b = ss.str();
	uint32_t c = 3;


	for(size_t i=0; i<1000; i++)
	{
		for( size_t i = 0; i<5; i++ )
		{
			a += 1;
			c += 10;

			std::string res = call.WaitInvokeRetAlertable<std::string>("TestThreads_HandlerWithCallBack");
			mass_return_if_not_equal(res, "TestThreads_SenderCallBack");
		}

		for(size_t i=0; i<5; i++)
		{
			a += 1;
			c += 10;

			std::string res = call.WaitInvokeRet<std::string>("TestThreads_Proc3", a, b, c);
			std::stringstream res_expect;
			res_expect<<a<<b<<c;
			mass_return_if_not_equal(res, res_expect.str());
		}

		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);

		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);

		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.AsyncInvoke("TestThreads_Proc3", a, b, c);
		a += 1;
		call.WaitInvoke("TestThreads_Proc3", a, b, c);
		a += 1;

	}
}

void TestThreads_MainThread()
{
	std::shared_ptr<IpcPackerTransportMemory> transport_buffer(new IpcPackerTransportMemory);
	std::shared_ptr<IpcPackerTransportMemory> complementar_buffer = transport_buffer->CreateComplementar();
	MAssIpcCall call_sender({});
	MAssIpcCall call_handler({});

	call_sender.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));
	call_sender.SetTransport(transport_buffer);

	call_sender.AddHandler("TestThreads_SenderCallBack", std::function<std::string()>(std::bind(&TestThreads_SenderCallBack, call_sender)));



	call_handler.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));
	call_handler.SetTransport(complementar_buffer);

	call_handler.AddHandler("TestThreads_Proc3", std::function<std::string(uint8_t, std::string, uint32_t)>(&TestThreads_Proc3));
	call_handler.AddHandler("TestThreads_HandlerWithCallBack", std::function<std::string()>(std::bind(&TestThreads_HandlerWithCallBack, call_handler)));


	std::vector<std::unique_ptr<std::thread>> sender_threads;
	std::vector<std::unique_ptr<std::thread>> handler_threads;
	for(size_t i=0; i<16; i++)
		sender_threads.push_back(std::unique_ptr<std::thread>(new std::thread(&TestThreads_Sender, transport_buffer, call_sender)));

	for( size_t i = 0; i<16; i++ )
		handler_threads.push_back(std::unique_ptr<std::thread>(new std::thread(&TestThreads_Handler, complementar_buffer, call_handler)));

	for( size_t i = 0; i<sender_threads.size(); i++ )
		sender_threads[i]->join();

	s_test_threads_run = false;
	complementar_buffer->CancelWaitRespond();

	for( size_t i = 0; i<handler_threads.size(); i++ )
		handler_threads[i]->join();
}

//-------------------------------------------------------
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

static void Ipc_Proc4()
{
}

static void ServerStop(bool* run)
{
	*run = false;
}


struct DataStruct1
{
	int a;
	int b;
	int c;
};


void Ipc_UniquePtr1(std::unique_ptr<DataStruct1> data)
{
}

MASS_IPC_TYPE_SIGNATURE(std::unique_ptr<DataStruct1>)

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const std::unique_ptr<DataStruct1>& v)
{
	return stream<<v->a<<v->b<<v->c;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, std::unique_ptr<DataStruct1>& v)
{
	v.reset(new DataStruct1);
	return stream>>v->a>>v->b>>v->c;
}


struct DataStruct2
{
	int a;
	int b;
	int c;
};


void Ipc_Ptr2(DataStruct2* data)
{
	delete data;
}

MASS_IPC_TYPE_SIGNATURE(DataStruct2*)

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const DataStruct2* v)
{
	return stream<<v->a<<v->b<<v->c;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, DataStruct2*& v)
{
	v = new DataStruct2;
	return stream>>v->a>>v->b>>v->c;
}

void Main_IpcService(std::shared_ptr<IpcPackerTransportMemory> transport_buffer)
{
	std::shared_ptr<MAssCallThreadTransport_Stub> thread_transport(new MAssCallThreadTransport_Stub);
	MAssIpcCall call(thread_transport);
	bool run = true;

	call.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));

	// 	call.AddHandler("Ipc_Proc1", DelegateW<std::string(uint8_t, std::string, uint32_t)>().BindS(&Ipc_Proc1));
	call.AddHandler("IsLinkUp_Sta", std::function<bool()>(&IsLinkUp), MAssThread::Id(3));
	call.AddHandler("Ipc_Proc4", std::function<void()>(&Ipc_Proc4));
	call.AddHandler("Ipc_Proc1", std::function<std::string(uint8_t, std::string, uint32_t)>(&Ipc_Proc1));
	call.AddHandler("Ipc_Proc3", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
	call.AddHandler("Ipc_UniquePtr1", std::function<void(std::unique_ptr<DataStruct1>)>(&Ipc_UniquePtr1));
	call.AddHandler("Ipc_UniquePtr2", std::function<void(DataStruct2*)>(&Ipc_Ptr2));
	//	call.AddHandler("IsLinkUp_Sta", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));

	call.AddHandler("ServerStop", std::function<void()>(std::bind(&ServerStop, &run)));

	call.SetTransport(transport_buffer);


	while( true )
	{
		call.ProcessTransport();
		if( !run )
			break;
		transport_buffer->WaitIncomingData();
	}
}

//-------------------------------------------------------

enum struct TestEnum1: uint8_t
{
	te1_a,
	te1_b,
};


MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const TestEnum1& v)
{
	stream<<std::underlying_type<TestEnum1>::type(v);
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, TestEnum1& v)
{
	return stream >> reinterpret_cast<std::underlying_type<TestEnum1>::type&>(v);
}

MASS_IPC_TYPE_SIGNATURE(TestEnum1);


void ClientProc(uint8_t a, uint32_t b)
{
	volatile int val = a+b;
	val = val;
}

//-------------------------------------------------------

void Main_IpcClient()
{
	MAssIpcCall call({});

// 	call.AddHandler("ClientProc", DelegateW<void(uint8_t, uint32_t)>().BindS(&ClientProc));

	call.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));

	std::shared_ptr<IpcPackerTransportMemory> transport_buffer(new IpcPackerTransportMemory);

	std::shared_ptr<IpcPackerTransportMemory> complementar_buffer = transport_buffer->CreateComplementar();
	std::thread serv_thread(&Main_IpcService, complementar_buffer);

	call.SetTransport(transport_buffer);


	uint8_t a = 123;
	std::string b = "4567";
	uint32_t c = 89012345;
	bool br2 = call.WaitInvokeRet<bool>("IsLinkUp_Sta");
	std::string res2 = call.WaitInvokeRet<std::string>("Ipc_Proc1",a, b, c);
	std::string res3 = call.WaitInvokeRet<std::string>("NotExistProc", a, b, c);
	call.AsyncInvoke("NotExistProc", a, b, c);
	uint8_t& a1 = a;
	call.WaitInvoke("Ipc_Proc3", a1, b, c);
	call.AsyncInvoke("Ipc_Proc3", a, b, c);
	call.WaitInvoke("Ipc_Proc3", TestEnum1::te1_a);
// 	TestStruct tstr;
// 	call.AsyncInvoke("TestProc", tstr);
	MAssIpcCall_EnumerateData enum_data = call.EnumerateRemote();
	MAssIpcCall_EnumerateData enum_data2 = call.EnumerateRemote();
	bool br = call.WaitInvokeRet<bool>("IsLinkUp_Sta");
	std::string res = call.WaitInvokeRet<std::string>("Ipc_Proc1",a, b, c);
	call.WaitInvoke("Ipc_Proc4");

	std::unique_ptr<DataStruct1> ds1(new DataStruct1({1,2,3}));
	DataStruct2* ds2 = new DataStruct2({4,5,6});
	call.WaitInvoke("Ipc_UniquePtr1", ds1);
	call.WaitInvoke("Ipc_UniquePtr2", ds2);


	call.WaitInvoke("ServerStop");

// 	TestThreads_MainThread();

// 	bool br = call["IsLinkUp_Sta"]();
// 	std::string res = call["Ipc_Proc1"](a, b, c);
//	res = std::string(call["Ipc_Proc1"](uint8_t(1), b, c));
	res = res;

	serv_thread.join();
}


//-------------------------------------------------------
// template<const char* str>
// struct  string
// {
// };
// 
// constexpr const char str[] = "str";
// string<str> test;
//-------------------------------------------------------

int main()
{
 	Main_IpcClient();
	TestThreads_MainThread();
	return 0;
}


