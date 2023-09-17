#include <vector>
#include <cstring>
#include <atomic>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <condition_variable>
#include <list>
#include <map>
#include <queue>
#include <future>

static constexpr const char* general_includes_end_mark = {};// help generating preprocessed to file cpp


//#include <functional>

static constexpr const char* main_start_mark = {};// help generating preprocessed to file cpp

#include "MAssIpcCall.h"
#include "MAssIpc_Macros.h"

//-------------------------------------------------------

static constexpr const char* main_mark_before_include_functional_is_bind_expression_begin = {};// help generating preprocessed to file cpp


template<bool expect_t, class TDelegate>
void AddHandler(const TDelegate& del)
{
	static_assert(MAssIpcCallInternal::Check_is_bind_expression<TDelegate>::value==expect_t, "bint NOT match");
}
void before_include_functional_is_bind_expression_CheckCompile()
{
	AddHandler<false>([](){});
}

#include <functional>

void after_include_functional_is_bind_expression_CheckCompile()
{
	AddHandler<true>(std::bind([](){}));
	AddHandler<false>([](){});
}

static constexpr const char* main_mark_before_include_functional_is_bind_expression_end = {};// help generating preprocessed to file cpp

//-------------------------------------------------------

#include "IpcTransport_MemoryCopy.h"
#include "IpcTransport_MemoryShare.h"
#include "IpcTransthread_Memory.h"
#include "TaskRunnerThread.h"

void CheckCompilation()
{ 
	MAssIpcCall call{{}};

	{
		struct Handler
		{
			void operator()() const{}
			operator bool() const{return true;}
		} const_handler;
		call.AddHandler("Handler", const_handler);
	}

	{
		struct Handler
		{
			void operator()() const{}
			operator bool(){return true;}
		} mutable_handler;
		call.AddHandler("Handler", mutable_handler);
	}

	{
		struct Handler
		{
			void operator()(){}
			void MemberFunc() const{}
		} no_bool_handler;
		call.AddHandler("Handler", no_bool_handler);
	}

	{
		struct Handler
		{
			void MemberFunc() const{}
			static void StaticFunc(){}
		};
		// call.AddHandler("Handler", &Handler::MemberFunc);// compilation error
		call.AddHandler("Handler", &Handler::StaticFunc);
	}

 	call.AddHandler("Handler", []()mutable{});
 	call.AddHandler("Handler", [](){});

	call.SetHandler_CallCountChanged([](const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info){});
	call.SetHandler_ErrorOccured([](MAssIpcCall::ErrorType et, const std::string& message){});

	{
		struct Handler
		{
			void operator()(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info) const{}

			void MemberFunc(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info) const{}

		} handler;
		call.SetHandler_CallCountChanged(handler);
		// call.SetHandler_CallCountChanged(&Handler::MemberFunc, &handler);// compilation error
	}

	{
		struct Handler
		{
			Handler() = default;
			Handler(Handler&& other) = default;

			Handler(const Handler&) = delete;
			Handler& operator= (const Handler&) = delete;

			void operator()() const{}
			void MemberFunc() const{}

		} handler;
		call.AddHandler("name", handler);
		call.AddHandler("name", Handler());
		std::unique_ptr<int> iptr{new int()};
#if __cplusplus >= 201402L
		call.AddHandler("name", [iptr = std::move(iptr)]()mutable{});
#endif
	}

	{
		struct Handler
		{
			Handler() = default;
			Handler(Handler&& other) = default;

			Handler(const Handler&) = delete;
			Handler& operator= (const Handler&) = delete;

			void operator()(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info) const{}
			void MemberFunc() const{}

		} handler;
		call.SetHandler_CallCountChanged(handler);
		call.SetHandler_CallCountChanged(Handler());
		std::unique_ptr<int> iptr{new int()};
#if __cplusplus >= 201402L
		call.SetHandler_CallCountChanged([iptr = std::move(iptr)](const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info){});
#endif
	}

	{
		struct Handler
		{
			Handler() = default;
			Handler(Handler&& other) = default;

			Handler(const Handler&) = delete;
			Handler& operator= (const Handler&) = delete;

			void operator()(MAssIpcCall::ErrorType et, const std::string& message) const{}
			void MemberFunc() const{}

		} handler;
		call.SetHandler_ErrorOccured(handler);
		call.SetHandler_ErrorOccured(Handler());
		std::unique_ptr<int> iptr{new int()};
#if __cplusplus >= 201402L
		call.SetHandler_ErrorOccured([iptr = std::move(iptr)](MAssIpcCall::ErrorType et, const std::string& message){});
#endif
	}

	{
		struct Handler
		{
			bool MemberFunc(float) const{return true;}
			static bool StaticFunc(float){return true;}
		};

		auto lambda_proc = [](float)->bool{return true; };

		constexpr const MAssIpcCall::SigName<bool(float)> sig_1 = {"sig_1"};
		constexpr const MAssIpcCall::SigName<bool(*)(float)> sig_2 = {"sig_2"};
		constexpr const MAssIpcCall::SigName<decltype(&Handler::MemberFunc)> sig_3 = {"sig_3"};
		constexpr const MAssIpcCall::SigName<decltype(&Handler::StaticFunc)> sig_4 = {"sig_4"};
		constexpr const MAssIpcCall::SigName<decltype(lambda_proc)> sig_5 = {"sig_5"};

		static_assert(std::is_same<decltype(sig_1)::SigProc, decltype(sig_2)::SigProc>::value, "must be same type");
		static_assert(std::is_same<decltype(sig_1)::SigProc, decltype(sig_3)::SigProc>::value, "must be same type");
		static_assert(std::is_same<decltype(sig_1)::SigProc, decltype(sig_4)::SigProc>::value, "must be same type");
		static_assert(std::is_same<decltype(sig_1)::SigProc, decltype(sig_5)::SigProc>::value, "must be same type");

		auto call_info1 = call.AddCallInfo(sig_1);
		auto call_info2 = call.AddHandler(sig_1, std::function<decltype(Handler::StaticFunc)>(&Handler::StaticFunc), {}, {}, {});
		auto call_info3 = call.AddHandler(sig_1, &Handler::StaticFunc, {}, {}, {});
	}
}

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


class NoDefConstruct
{
public:
	NoDefConstruct() = delete;
	NoDefConstruct(const NoDefConstruct&) = default;
	NoDefConstruct(uint32_t a)
		:m_a(a)
	{
	}

	~NoDefConstruct()
	{
		// volatile int a = 0;
	}

	uint32_t m_a;
};


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const NoDefConstruct& v)
{
	stream<<v.m_a;
	return stream;
}

NoDefConstruct operator>>(MAssIpc_DataStream& stream, const MAssIpc_DataStream_Create<NoDefConstruct>& v)
{
	uint32_t a{};
	stream>>a;
	return NoDefConstruct(a);
}

MASS_IPC_TYPE_SIGNATURE(NoDefConstruct);


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const std::unique_ptr<NoDefConstruct>& v)
{
	stream<<v->m_a;
	return stream;
}

std::unique_ptr<NoDefConstruct> operator>>(MAssIpc_DataStream& stream, const MAssIpc_DataStream_Create<std::unique_ptr<NoDefConstruct>>& v)
{
	uint32_t a{};
	stream>>a;
	return std::unique_ptr<NoDefConstruct>{new NoDefConstruct(a)};
}
MASS_IPC_TYPE_SIGNATURE(std::unique_ptr<NoDefConstruct>);


// static_assert(IsReadStreamCreating<NoDefConstruct>::value, "NoDefConstruct should have a stream operator");
// static_assert(!IsReadStreamCreating<int>::value, "int should not have a stream operator");



static MAssIpc_TransthreadTarget::Id CreateCustomId(size_t value)
{
	MAssIpc_TransthreadTarget::Id id;
	static_assert(sizeof(id)==sizeof(value), "unexpected size");
	size_t* id_value = reinterpret_cast<size_t*>(&id);
	*id_value = value;
	return id;
}

//-------------------------------------------------------
class MAssCallThreadTransport_Stub: public MAssIpc_Transthread
{
public:

	void			CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job) override
	{
		job->Invoke();
	}

	MAssIpc_TransthreadTarget::Id	GetResultSendThreadId() override
	{
		return CreateCustomId(2);
	}
};

//-------------------------------------------------------

static MAssIpcCall::ErrorType s_state_error_et;
static std::string s_state_error_message;

static void OnInvalidRemoteCall(MAssIpcCall::ErrorType et, const std::string& message)
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

void TestThreads_Handler(std::shared_ptr<IpcTransport_MemoryShare> transport_buffer, MAssIpcCall call)
{
	while( true )
	{
		call.ProcessTransport();
		if( !s_test_threads_run )
			break;
// 		transport_buffer->WaitIncomingData();
	}
}

void TestThreads_Sender(std::shared_ptr<IpcTransport_MemoryShare> transport_buffer, MAssIpcCall call)
{
	std::stringstream ss;
	ss<<MAssIpc_ThreadSafe::get_id();

	uint8_t a = 1;
	std::string b = ss.str();
	uint32_t c = 3;


	for(size_t i=0; i<1000; i++)
	{
		for( size_t i = 0; i<5; i++ )
		{
			a += 1;
			c += 10;

			std::string res = call.WaitInvokeRet<std::string>({"TestThreads_HandlerWithCallBack",MAssIpcCall::ProcIn::now});
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
	std::shared_ptr<IpcTransport_MemoryShare> transport_buffer(new IpcTransport_MemoryShare);
	std::shared_ptr<IpcTransport_MemoryShare> complementar_buffer = transport_buffer->CreateComplementar();
	MAssIpcCall call_sender({});
	MAssIpcCall call_handler({});

	call_sender.SetHandler_ErrorOccured(&OnInvalidRemoteCall);
	call_sender.SetTransport(transport_buffer);

	call_sender.AddHandler("TestThreads_SenderCallBack", std::function<std::string()>(std::bind(&TestThreads_SenderCallBack, call_sender)));



	call_handler.SetHandler_ErrorOccured(&OnInvalidRemoteCall);
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

static NoDefConstruct Proc_NoDefConstruct(NoDefConstruct ndc)
{
	return NoDefConstruct(ndc.m_a*2);
}

static std::unique_ptr<NoDefConstruct> Proc_NoDefConstruct_unique_ptr(std::unique_ptr<NoDefConstruct> ndc)
{
	ndc->m_a *= 4;
	return std::move(ndc);
}

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

template<typename TVal>
std::vector<TVal> VectorT(std::vector<TVal> val)
{
	std::vector<TVal> ret(val.rbegin(), val.rend());
	return ret;
}

template<typename TVal>
std::vector<TVal> MakeVectorT()
{
	std::vector<TVal> ret;
	ret.resize(10);
	for( size_t i = 0; i<ret.size(); i++ )
		ret[i] = TVal(i+1);

	return ret;
}

template<typename TVal>
void CheckVectorT(const std::vector<TVal>& val)
{
	for( size_t i = 0; i<val.size(); i++ )
		mass_return_if_not_equal(val[i], TVal(10-i));
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

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const std::unique_ptr<DataStruct1>& v)
{
	return stream<<v->a<<v->b<<v->c;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, std::unique_ptr<DataStruct1>& v)
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

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const DataStruct2* v)
{
	return stream<<v->a<<v->b<<v->c;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, DataStruct2*& v)
{
	v = new DataStruct2;
	return stream>>v->a>>v->b>>v->c;
}

static std::vector<std::string> s_calls;
static std::mutex s_calls_access;

void CallCountChanged_BeforeInvoke()
{
	std::unique_lock<std::mutex> lock(s_calls_access, std::try_to_lock);
	mass_return_if_not_equal(lock.owns_lock(), true);
	s_calls.push_back("CallCountChanged_BeforeInvoke");
}

void CallCountChanged(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
{
	uint32_t count = call_info->GetCallCount();
	std::string call_data = "CallCountChanged:" + std::to_string(count) + std::string(" ") + call_info->GetName();
//	std::unique_lock<std::mutex> lock(s_calls_access);

	std::unique_lock<std::mutex> lock(s_calls_access, std::try_to_lock);
	mass_return_if_not_equal(lock.owns_lock(), true);
	s_calls.push_back(call_data);
}

static std::string Static_String_StringU32Double(std::string s, uint32_t a, double b)
{
	return s + std::to_string(int(a*b));
}

class SigCheck
{
public:
	std::string Static_String_StringU32Double(std::string s, uint32_t a, double b)
	{
		return s + std::to_string(int(a*b));
	}
};

constexpr const MAssIpcCall::SigName<std::string(std::string, uint32_t, double)> sig_Static_String_StringU32Double = {"Float_StringDouble"};
constexpr const MAssIpcCall::SigName<decltype(&Static_String_StringU32Double)> sig2_Static_String_StringU32Double = {"Float_StringDouble"};
constexpr const MAssIpcCall::SigName<decltype(&SigCheck::Static_String_StringU32Double)> sig3_Static_String_StringU32Double = {"Float_StringDouble"};
static_assert(std::is_same<decltype(sig_Static_String_StringU32Double)::SigProc, decltype(sig2_Static_String_StringU32Double)::SigProc>::value, "must be same type");
static_assert(std::is_same<decltype(sig_Static_String_StringU32Double)::SigProc, decltype(sig3_Static_String_StringU32Double)::SigProc>::value, "must be same type");

void Main_IpcService(std::shared_ptr<IpcTransport_MemoryShare> transport_buffer)
{
	std::shared_ptr<MAssCallThreadTransport_Stub> thread_transport(new MAssCallThreadTransport_Stub);
	MAssIpcCall call(thread_transport);
	bool run = true;

	call.SetHandler_ErrorOccured(&OnInvalidRemoteCall);

	std::shared_ptr<const MAssIpcCall::CallInfo> call_info;

	call.AddHandler("Proc_NoDefConstruct", std::function<NoDefConstruct(NoDefConstruct)>(&Proc_NoDefConstruct));
	call.AddHandler("Proc_NoDefConstruct_unique_ptr", std::function<std::unique_ptr<NoDefConstruct>(std::unique_ptr<NoDefConstruct>)>(&Proc_NoDefConstruct_unique_ptr));
	
	auto call_info1 = call.AddCallInfo(sig_Static_String_StringU32Double);
	auto call_info2 = call.AddHandler(sig_Static_String_StringU32Double, std::function<decltype(Static_String_StringU32Double)>(&Static_String_StringU32Double), {}, {}, {});
	mass_return_if_not_equal(call_info1, call_info2);
	// call.AddHandler(sig_Static_String_StringU32Double, std::function<std::string(std::string, uint32_t, float)>(&Static_String_StringU32Double), {}, {}, {});// compile error

	// 	call.AddHandler("Ipc_Proc1", DelegateW<std::string(uint8_t, std::string, uint32_t)>().BindS(&Ipc_Proc1));
	call.AddHandler("IsLinkUp_Sta", std::function<bool()>(&IsLinkUp), CreateCustomId(3));
	call.AddHandler("Ipc_Proc4", std::function<void()>(&Ipc_Proc4));
	call.AddHandler("Ipc_Proc1", std::function<std::string(uint8_t, std::string, uint32_t)>(&Ipc_Proc1));
	call.AddHandler("Ipc_Proc3", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
	call.AddHandler("Ipc_UniquePtr1", std::function<void(std::unique_ptr<DataStruct1>)>(&Ipc_UniquePtr1));
	call.AddHandler("Ipc_UniquePtr2", std::function<void(DataStruct2*)>(&Ipc_Ptr2));
	call.AddHandler("CallCountChanged_BeforeInvoke",&CallCountChanged_BeforeInvoke);
	//	call.AddHandler("IsLinkUp_Sta", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));

	class TagLocal
	{
	}; 
	TagLocal* tag = nullptr;
	MAssIpc_TransthreadTarget::Id thread_id;
	const char* proc_name = "test compile 2";
	const char proc_name_literal[] = {"test compile 3"};
	call.AddHandler("test compile 1", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3), {}, {}, tag);// obsolate
	call.AddHandler(proc_name, std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
	call.AddHandler(proc_name_literal, std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
	call.AddHandler("test compile 4", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3));
	call.AddHandler("test compile 5", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3), tag);
	call.AddHandler("test compile 6", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3), tag, thread_id);
	call.AddHandler("test compile 7", std::function<void(uint8_t, std::string, uint32_t)>(&Ipc_Proc3), tag, thread_id, "comment");

	call.AddHandler("VectorU8", std::function<std::vector<uint8_t>(std::vector<uint8_t>)>(&VectorT<uint8_t>));
	call.AddHandler("VectorU16", std::function<std::vector<uint16_t>(std::vector<uint16_t>)>(&VectorT<uint16_t>));
	call.AddHandler("VectorU32", std::function<std::vector<uint32_t>(std::vector<uint32_t>)>(&VectorT<uint32_t>));
	call.AddHandler("VectorU64", std::function<std::vector<uint64_t>(std::vector<uint64_t>)>(&VectorT<uint64_t>));
	call.AddHandler("VectorI8", std::function<std::vector<int8_t>(std::vector<int8_t>)>(&VectorT<int8_t>));
	call.AddHandler("VectorI16", std::function<std::vector<int16_t>(std::vector<int16_t>)>(&VectorT<int16_t>));
	call.AddHandler("VectorI32", std::function<std::vector<int32_t>(std::vector<int32_t>)>(&VectorT<int32_t>));
	call.AddHandler("VectorI64", std::function<std::vector<int64_t>(std::vector<int64_t>)>(&VectorT<int64_t>));

	call.AddHandler("ServerStop", std::function<void()>(std::bind(&ServerStop, &run)));

	call.SetTransport(transport_buffer);

// 	call.SetHandler_CallCountChanged([](const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info){});
	call.SetHandler_CallCountChanged(&CallCountChanged);


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


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const TestEnum1& v)
{
	stream<<std::underlying_type<TestEnum1>::type(v);
	return stream;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, TestEnum1& v)
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


void StrProc(const MAssIpcCallInternal::MAssIpc_RawString& str)
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

	call.SetHandler_ErrorOccured(&OnInvalidRemoteCall);

	std::shared_ptr<IpcTransport_MemoryShare> transport_buffer(new IpcTransport_MemoryShare);

	std::shared_ptr<IpcTransport_MemoryShare> complementar_buffer = transport_buffer->CreateComplementar();
	std::thread serv_thread(&Main_IpcService, complementar_buffer);

	call.SetTransport(transport_buffer);

	{
		auto packet_size = MAssIpcCall::CalcCallSize<void>(true, "NotExistProc", 0);
		std::unique_ptr<MAssIpc_Data> ipc_buffer(new IpcTransport_MemoryShare::MemoryPacket(packet_size));
 		call.AsyncInvoke({"NotExistProc", std::move(ipc_buffer)}, 0);
// 		call.InvokeWait("NotExistProc", std::move(ipc_buffer)).RetArgs<int>();
//  	call[{std::move(ipc_buffer), "NotExistProc"}];
	}


	{
		NoDefConstruct s(123);
		NoDefConstruct r{call.WaitInvokeRet<NoDefConstruct>("Proc_NoDefConstruct", s)};
		mass_return_if_not_equal(r.m_a, s.m_a*2);

		std::unique_ptr<NoDefConstruct> us(new NoDefConstruct(123));
		std::unique_ptr<NoDefConstruct> ur{call.WaitInvokeRet<std::unique_ptr<NoDefConstruct>>("Proc_NoDefConstruct_unique_ptr", us)};
		mass_return_if_not_equal(ur->m_a, us->m_a*4);
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
	{
		std::unique_lock<std::mutex> lock(s_calls_access, std::try_to_lock);
		mass_return_if_not_equal(lock.owns_lock(), true);
		mass_return_if_not_equal(s_calls.empty(), false);
	}

	std::string str = call.WaitInvoke(sig_Static_String_StringU32Double, "str:", uint8_t(2), double(123.5));
	mass_return_if_not_equal(str, "str:247");


	c = 654321;
	const char* string_pointer_Ipc_Proc1 = "Ipc_Proc1";
	res2 = call.WaitInvokeRet<std::string>(string_pointer_Ipc_Proc1, a, b, c);
	mass_return_if_not_equal(res2, "Ipc_Proc1 result 1234567654321");
	mass_return_if_not_equal(s_state_error_et, MAssIpcCall::ErrorType::unknown_error);
	{
		std::unique_lock<std::mutex> lock(s_calls_access, std::try_to_lock);
		mass_return_if_not_equal(lock.owns_lock(), true);
		mass_return_if_not_equal(s_calls[s_calls.size()-1], "CallCountChanged:2 Ipc_Proc1");
	}

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

	std::vector<uint8_t> 	val_uint8_t  = call.WaitInvokeRet <std::vector<uint8_t> >("VectorU8", MakeVectorT<uint8_t>());
	std::vector<uint16_t>	val_uint16_t = call.WaitInvokeRet<std::vector<uint16_t>	>("VectorU16", MakeVectorT<uint16_t>());
	std::vector<uint32_t>	val_uint32_t = call.WaitInvokeRet<std::vector<uint32_t>	>("VectorU32", MakeVectorT<uint32_t>());
	std::vector<uint64_t>	val_uint64_t = call.WaitInvokeRet<std::vector<uint64_t>	>("VectorU64", MakeVectorT<uint64_t>());
	std::vector<int8_t> 	val_int8_t   = call.WaitInvokeRet  <std::vector<int8_t> >("VectorI8", MakeVectorT<int8_t>());
	std::vector<int16_t>	val_int16_t  = call.WaitInvokeRet <std::vector<int16_t>	>("VectorI16", MakeVectorT<int16_t>());
	std::vector<int32_t>	val_int32_t  = call.WaitInvokeRet <std::vector<int32_t>	>("VectorI32", MakeVectorT<int32_t>());
	std::vector<int64_t>	val_int64_t  = call.WaitInvokeRet <std::vector<int64_t>	>("VectorI64", MakeVectorT<int64_t>());
	CheckVectorT(val_uint8_t);
	CheckVectorT(val_uint16_t);
	CheckVectorT(val_uint32_t);
	CheckVectorT(val_uint64_t);
	CheckVectorT(val_int8_t);
	CheckVectorT(val_int16_t);
	CheckVectorT(val_int32_t);
	CheckVectorT(val_int64_t);

	for(size_t i=1; i<100; i++)
	{
		std::string expect_calls_1 = std::string("CallCountChanged:") + std::to_string(i) + std::string(" CallCountChanged_BeforeInvoke");
		s_calls.clear();
		call.WaitInvoke("CallCountChanged_BeforeInvoke");
		mass_return_if_not_equal(s_calls.size(), 2);
		mass_return_if_not_equal(s_calls[0], expect_calls_1);
		mass_return_if_not_equal(s_calls[1], "CallCountChanged_BeforeInvoke");
	}
	


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


