#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "../Ipc/MAssIpcCall.h"
#include <functional>
#include "../Ipc/MAssMacros.h"
#include <sstream>
#include <mutex>
#include <condition_variable>

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

// void* operator new(std::size_t n) throw(std::bad_alloc)
// {
// 	return malloc();
// }
// void operator delete(void* p) throw()
// {
// 	//...
// }

static MAssIpcThreadTransportTarget::Id CreateCustomId(size_t value)
{
	MAssIpcThreadTransportTarget::Id id;
	static_assert(sizeof(id)==sizeof(value), "unexpected size");
	size_t* id_value = reinterpret_cast<size_t*>(&id);
	*id_value = value;
	return id;
}


class IpcTransportMemory: public MAssIpcCallTransport
{
private:
	struct SyncData
	{
		std::function<void()> on_read;
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

	void SetHnadler_OnRead(std::function<void()> on_read, bool sync_data_read)
	{
		std::shared_ptr<SyncData>& sync_data = sync_data_read ? m_read : m_write;
		sync_data->on_read = on_read;
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
		if( sync.on_read )
			sync.on_read();// it not necessary but ipc must survive this
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
		std::function<void()> on_read;
		std::mutex	lock;
		bool incoming_data = false;
		bool cancel_wait_respond = false;
		std::condition_variable cond;
		std::list<std::unique_ptr<const MAssIpcData> > data;
	};

public:

	class MemoryPacket: public MAssIpcData
	{
	public:

		MemoryPacket(size_t size)
			:m_data(size)
		{
		}

	private:

		MAssIpcData::TPacketSize Size() const override
		{
			return m_data.size();
		}

		uint8_t* Data() override
		{
			return m_data.data();
		}

		const uint8_t* Data() const override
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

	void SetHnadler_OnRead(std::function<void()> on_read, bool sync_data_read)
	{
		std::shared_ptr<SyncData>& sync_data = sync_data_read ? m_read : m_write;
		sync_data->on_read = on_read;
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

	std::unique_ptr<MAssIpcData> Create(MAssIpcData::TPacketSize size) override
	{
		return std::unique_ptr<MAssIpcData>(new MemoryPacket(size));
	}

	bool	Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpcData>* packet) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		if( m_read->on_read )
			m_read->on_read();// it not necessary but ipc must survive this
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

	void	Write(std::unique_ptr<const MAssIpcData> packet) override
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

	void			CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<Job> job) override
	{
		job->Invoke();
	}

	MAssIpcThreadTransportTarget::Id	GetResultSendThreadId() override
	{
		return CreateCustomId(2);
	}
};

//-------------------------------------------------------

static MAssIpcCall::ErrorType s_state_error_et;
static std::string s_state_error_message;

static void OnInvalidRemoteCall(MAssIpcCall::ErrorType et, std::string message)
{
	s_state_error_et = et;
	s_state_error_message = message;
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
	ss<<MAssIpcThreadSafe::get_id();

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

	transport_buffer->SetHnadler_OnRead([&](){call_sender.ProcessTransport();}, true);
	transport_buffer->SetHnadler_OnRead([&](){call_handler.ProcessTransport();}, false);

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

	return std::string("Ipc_Proc1 result ")+std::to_string(a)+b+std::to_string(c);
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
	call.AddHandler("IsLinkUp_Sta", std::function<bool()>(&IsLinkUp), CreateCustomId(3));
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

void StrProc(const MAssIpcCallInternal::MAssIpcRawString& str)
{
}

void Main_IpcClient()
{
	MAssIpcCall call({});

	const char str1[] = {"asdf"};
	const char* str2 = {"asdf"};

	StrProc(str1);
  	StrProc(str2);

// 	call.AddHandler("ClientProc", DelegateW<void(uint8_t, uint32_t)>().BindS(&ClientProc));

	call.SetErrorHandler(MAssIpcCall::TErrorHandler(&OnInvalidRemoteCall));

	std::shared_ptr<IpcPackerTransportMemory> transport_buffer(new IpcPackerTransportMemory);

	std::shared_ptr<IpcPackerTransportMemory> complementar_buffer = transport_buffer->CreateComplementar();
	std::thread serv_thread(&Main_IpcService, complementar_buffer);

	call.SetTransport(transport_buffer);

	{
		auto packet_size = MAssIpcCall::CalcCallSize<void>(true, "NotExistProc", 0);
		std::unique_ptr<MAssIpcData> ipc_buffer(new IpcPackerTransportMemory::MemoryPacket(packet_size));
 		call.AsyncInvoke({"NotExistProc", std::move(ipc_buffer)}, 0);
// 		call.InvokeWait("NotExistProc", std::move(ipc_buffer)).RetArgs<int>();
//  	call[{std::move(ipc_buffer), "NotExistProc"}];
	}


	uint8_t a = 123;
	std::string b = "4567";
	uint32_t c = 89012345;
	bool br2 = call.WaitInvokeRet<bool>("IsLinkUp_Sta");
	br2 = br2;
	s_state_error_et = MAssIpcCall::ErrorType::unknown_error;
	std::string res2 = call.WaitInvokeRet<std::string>("Ipc_Proc1",a, b, c);
	mass_return_if_not_equal(res2, "Ipc_Proc1 result 123456789012345");
	mass_return_if_not_equal(s_state_error_et, MAssIpcCall::ErrorType::unknown_error);

	c = 654321;
	const char* string_pointer_Ipc_Proc1 = "Ipc_Proc1";
	res2 = call.WaitInvokeRet<std::string>(string_pointer_Ipc_Proc1, a, b, c);
	mass_return_if_not_equal(res2, "Ipc_Proc1 result 1234567654321");
	mass_return_if_not_equal(s_state_error_et, MAssIpcCall::ErrorType::unknown_error);

	s_state_error_et = MAssIpcCall::ErrorType::unknown_error;
	std::string res3 = call.WaitInvokeRet<std::string>("NotExistProc", a, b, c);
	mass_return_if_not_equal(s_state_error_et, MAssIpcCall::ErrorType::respond_no_matching_call_name_parameters);
	s_state_error_et = MAssIpcCall::ErrorType::unknown_error;
	int res4 = call.WaitInvokeRet<int>("Ipc_Proc1", a, b, c);
	mass_return_if_not_equal(s_state_error_et, MAssIpcCall::ErrorType::respond_no_matching_call_return_type);
	res4 = res4;
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
	br = br;
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


