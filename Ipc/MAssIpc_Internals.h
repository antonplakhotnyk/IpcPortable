#pragma once

#include "MAssIpc_DataStream.h"
#include "MAssIpc_Transthread.h"
#include "MAssIpc_PacketParser.h"
#include "MAssIpc_Transport.h"
#include "MAssIpc_ThreadSafe.h"
#include "MAssIpc_RawString.h"
#include <map>
#include <memory>// important include before __cpp_lib_atomic_shared_ptr
#include <atomic>
#include <type_traits>

//-------------------------------------------------------

#include <functional>
// namespace std
// {
// template< class T >
// struct is_bind_expression;
// }

namespace MAssIpcImpl
{

template<class T>
auto IsAvailable_is_bind_expression(int) -> decltype(std::is_bind_expression<T>{});

template<class>
auto IsAvailable_is_bind_expression(...) -> std::false_type;

template<class T>
class Check_is_bind_expression: public decltype(IsAvailable_is_bind_expression<T>(0))
{
};

}

//-------------------------------------------------------

namespace MAssIpcImpl
{

class AtomicSharedPtr_autotest_start_mark_begin;

#if __cpp_lib_atomic_shared_ptr

template <typename T>
class AtomicSharedPtr
{
public:
	AtomicSharedPtr(std::shared_ptr<T> initial = std::shared_ptr<T>()): m_atomic_ptr(initial)
	{
	}

	AtomicSharedPtr(const AtomicSharedPtr<T>& other)
	{
		m_atomic_ptr.store(other.m_atomic_ptr.load());
	}

	AtomicSharedPtr<T>& operator=(const AtomicSharedPtr<T>& other)
	{
		if( this == &other )
			return *this;

		m_atomic_ptr.store(other.m_atomic_ptr.load());
		return *this;
	}

	AtomicSharedPtr(AtomicSharedPtr<T>&& other)
	{
		std::shared_ptr<T> tmp = other.m_atomic_ptr.exchange(std::shared_ptr<T>());
		m_atomic_ptr.store(tmp);
	}

	AtomicSharedPtr<T>& operator=(AtomicSharedPtr<T>&& other)
	{
		if( this == &other )
			return *this;

		std::shared_ptr<T> tmp = other.m_atomic_ptr.exchange(std::shared_ptr<T>());
		m_atomic_ptr.store(tmp);
		return *this;
	}

	void store(const std::shared_ptr<T>& desired)
	{
		m_atomic_ptr.store(desired);
	}

	std::shared_ptr<T> load() const
	{
		return m_atomic_ptr.load();
	}

private:
	std::atomic<std::shared_ptr<T>> m_atomic_ptr = {};
};

#else

template <typename T>
class AtomicSharedPtr
{
public:
	AtomicSharedPtr(std::shared_ptr<T> initial = std::shared_ptr<T>()): m_ptr(initial)
	{
	}

	AtomicSharedPtr(const AtomicSharedPtr<T>& other)
	{
		std::atomic_store(&m_ptr, std::atomic_load(&other.m_ptr));
	}

	AtomicSharedPtr& operator=(const AtomicSharedPtr<T>& other)
	{
		if( this == &other )
			return *this;

		std::atomic_store(&m_ptr, std::atomic_load(&other.m_ptr));
		return *this;
	}

	AtomicSharedPtr(AtomicSharedPtr<T>&& other)
	{
		std::shared_ptr<T> tmp = std::atomic_exchange(&other.m_ptr, std::shared_ptr<T>());
		std::atomic_store(&m_ptr, tmp);
	}

	AtomicSharedPtr& operator=(AtomicSharedPtr<T>&& other)
	{
		if( this == &other )
			return *this;

		std::shared_ptr<T> tmp = std::atomic_exchange(&other.m_ptr, std::shared_ptr<T>());
		std::atomic_store(&m_ptr, tmp);
		return *this;
	}

	void ltore(const std::shared_ptr<T>& desired)
	{
		std::atomic_store(&m_ptr, desired);
	}

	std::shared_ptr<T> load() const
	{
		return std::atomic_load(&m_ptr);
	}

private:
	mutable std::shared_ptr<T> m_ptr;
};

#endif


class AtomicSharedPtr_autotest_start_mark_end;

}
//-------------------------------------------------------


namespace MAssIpcImpl
{

MAssIpc_DataStream CreateDataStream(const std::weak_ptr<MAssIpc_TransportShare>& weak_transport,
									   MAssIpc_Data::PacketSize no_header_size,
									   MAssIpc_PacketParser::PacketType pt,
									   MAssIpc_PacketParser::CallId respond_id);

MAssIpc_DataStream CreateDataStreamInplace(std::unique_ptr<MAssIpc_Data> inplace_send_buffer,
											  MAssIpc_Data::PacketSize no_header_size,
											  MAssIpc_PacketParser::PacketType pt,
											  MAssIpc_PacketParser::CallId respond_id);

//-------------------------------------------------------

template<typename Signature, typename = void>
struct FuncSigSilent
{
	using IsAssert = std::true_type;
	using IsHandlerType = std::false_type;
};

template<typename Ret, typename... Args>
struct FuncSigSilent< Ret(Args...)>
{
	using IsAssert = std::false_type;

	using FuncType = Ret(Args...);
	using FuncPtr = Ret(*)(Args...);
	using FuncRet = Ret;
};

template<typename Ret, typename... Args>
struct FuncSigSilent<Ret(*)(Args...)>: public FuncSigSilent<Ret(Args...)>
{
	using IsHandlerType = std::true_type;
};

template<typename Ret, typename... Args>
struct FuncSigSilent<Ret(*const)(Args...)>: public FuncSigSilent<Ret(Args...)>
{
	using IsHandlerType=std::true_type;
};

template<typename Class, typename Ret, typename... Args>
struct FuncSigSilent<Ret(Class::*)(Args...)>: public FuncSigSilent<Ret(Args...)>
{
	using IsHandlerType = std::false_type;
};

template<typename Class, typename Ret, typename... Args>
struct FuncSigSilent<Ret(Class::*)(Args...) const>: public FuncSigSilent<Ret(Args...)>
{
	using IsHandlerType = std::false_type;
};

template<typename Callable>
struct FuncSigSilent<Callable, typename std::enable_if<std::is_member_function_pointer<decltype(&std::decay<Callable>::type::operator())>::value>::type>
	: public FuncSigSilent<decltype(&std::decay<Callable>::type::operator())>
{
	using IsHandlerType = std::true_type;
};

template<typename Signature>
struct FuncSig: FuncSigSilent<Signature>
{
	static_assert(!FuncSigSilent<Signature>::IsAssert::value, "specialization not found, Signature must be member or static function pointer like decltype(&Cls::Proc) or function like Ret(T1,T2) or callable with operator()");
};

template<class THandler>
struct IsHandlerTypeCompatible: FuncSig<THandler>::IsHandlerType
{
};

//-------------------------------------------------------

template<class From, class To>
auto ConversionAvailable(int) -> decltype(To(std::declval<From>()), std::true_type{});

template<class, class>
auto ConversionAvailable(...) -> std::false_type;

template<class From, class To>
class IsConvertible: public decltype(ConversionAvailable<From, To>(0)){};

struct IsCallable
{
	template<class Delegate>
	using FuncPtr = typename FuncSigSilent<Delegate>::FuncPtr;

	template<class Delegate>
	using IsPointer = std::is_convertible<Delegate, FuncPtr<Delegate>>;

    template<class Delegate, typename std::enable_if<IsConvertible<Delegate, bool>::value && !IsPointer<Delegate>::value, bool>::type=true>
    static constexpr bool Convertible(Delegate& del) {return bool(del);}

    template<class Delegate, typename std::enable_if<IsPointer<Delegate>::value, bool>::type=true>
    static constexpr bool Convertible(Delegate& del) {return !(FuncPtr<Delegate>(del)==nullptr);}

    template<class Delegate, typename std::enable_if<!IsConvertible<Delegate, bool>::value && !IsPointer<Delegate>::value, bool>::type=true>
    static constexpr bool Convertible(Delegate& del) {return true;}
};

//-------------------------------------------------------

template<class Ret, class... Args>
static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(Ret(*a1)(Args...), Ret(*a2)(Args...))
{
	return true;
};

template<class A1, class A2>
static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(A1, A2, ...)
{
	static_assert(std::is_same<A1, A2>::value, "A1 and A2 must be same types");
	return false;
};

//-------------------------------------------------------


class CallInfo
{
public:

	using TCounter = uint32_t;

	virtual ~CallInfo()=default;
	virtual TCounter GetCallCount() const = 0;
	virtual const std::string& GetName() const = 0;
};

enum struct ErrorType: uint8_t
{
	unknown_error,
	no_matching_call_name_parameters,
	no_matching_call_return_type,
	respond_no_matching_call_name_parameters,
	respond_no_matching_call_return_type,
};

static constexpr const char* ErrorType_string_map[] =
{
	"unknown_error",
	"no_matching_call_name_parameters",
	"no_matching_call_return_type",
	"respond_no_matching_call_name_parameters",
	"respond_no_matching_call_return_type",
};

static inline constexpr const char* ErrorTypeToStr(ErrorType error)
{
	static_assert(size_t(ErrorType::unknown_error) == 0, "unexpected value");
	return (size_t(error) < std::extent<decltype(ErrorType_string_map)>::value) ? ErrorType_string_map[size_t(error)] : ErrorType_string_map[0];
};


class InvokeBase
{
public:
	InvokeBase(MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		:m_thread_id(thread_id)
		, m_tag(tag)
	{
	}

	virtual ~InvokeBase()=default;
	virtual bool IsCallable() const = 0;

public:

	const void* const	m_tag = nullptr;
	const MAssIpc_TransthreadTarget::Id m_thread_id;
};

class InvokeRemoteBase: public InvokeBase
{
public:

	InvokeRemoteBase(MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		:InvokeBase(thread_id, tag)
	{
	}

	virtual std::unique_ptr<const MAssIpc_Data> Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport,
													   MAssIpc_PacketParser::CallId respond_id,
													   MAssIpc_DataStream& params) const = 0;
	virtual MAssIpc_RawString GetSignature_ReturnType() const = 0;
};

class CallInfoImpl: public CallInfo
{
public:
	CallInfoImpl(const MAssIpc_RawString& proc_name, std::string params_type)
		: m_name(proc_name.Std_String())
		, m_params_type(std::move(params_type))
	{
	};


	struct SignatureKey
	{
		MAssIpc_RawString name;
		MAssIpc_RawString params_type;

		bool operator<(const SignatureKey& other) const
		{
			if( name == other.name )
				return (params_type<other.params_type);
			else
				return name<other.name;
		}
	};
	SignatureKey GetSignatureKey() const
	{
		return SignatureKey{m_name,m_params_type};
	}

	template<class Delegate>
	static std::string MakeParamsType();

	void IncrementCallCount()
	{
		m_call_count++;
	}

	uint32_t GetCallCount() const override
	{
		return m_call_count;
	}

	const std::string& GetName() const override
	{
		return m_name;
	}

	void SetInvoke(std::unique_ptr<InvokeRemoteBase> invoke)
	{
		m_invoke = std::move(invoke);
	}

	const std::shared_ptr<InvokeRemoteBase> IsInvokable() const
	{
		if( m_invoke && m_invoke->IsCallable() )
			return m_invoke;
		return {};
	}

protected:

	std::shared_ptr<InvokeRemoteBase> m_invoke;

	MAssIpc_ThreadSafe::atomic_uint32_t	m_call_count = {0};
	const std::string		m_name;
	const std::string		m_params_type;
};


class ResultJob: public MAssIpc_Transthread::Job
{
public:

	ResultJob(const std::weak_ptr<MAssIpc_TransportShare>& transport, std::unique_ptr<const MAssIpc_Data>& result)
		: m_transport(transport)
		, m_result(std::move(result))
	{
	}

	void Invoke() override
	{
		Invoke(m_transport, m_result);
	}

	static void Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, std::unique_ptr<const MAssIpc_Data>& result);

private:

	std::weak_ptr<MAssIpc_TransportShare> m_transport;
	std::unique_ptr<const MAssIpc_Data> m_result;
};


class CallJob: public MAssIpc_Transthread::Job
{
public:

	CallJob(const std::weak_ptr<MAssIpc_TransportShare>& transport, 
			const std::weak_ptr<MAssIpc_Transthread>& inter_thread,
			MAssIpc_DataStream& call_info_data, MAssIpc_PacketParser::CallId respond_id,
			std::shared_ptr<const InvokeRemoteBase> invoke_base)
		: m_call_info_data(std::move(call_info_data))
		, m_transport(transport)
		, m_inter_thread(inter_thread)
		, m_respond_id(respond_id)
		, m_invoke_base(invoke_base)
	{
	}

	void Invoke() override;
	static void Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread,
					   MAssIpc_DataStream& call_info_data, std::shared_ptr<const InvokeRemoteBase> invoke_base,
					   MAssIpc_PacketParser::CallId respond_id);
	static MAssIpc_PacketParser::CallId CalcRespondId(bool send_return, MAssIpc_PacketParser::CallId respond_id);

	MAssIpc_DataStream					m_call_info_data;
	const std::shared_ptr<const InvokeRemoteBase>	m_invoke_base;
	const MAssIpc_PacketParser::CallId		m_respond_id;

private:

	const std::weak_ptr<MAssIpc_TransportShare>	m_transport;
	const std::weak_ptr<MAssIpc_Transthread>	m_inter_thread;
};


//-------------------------------------------------------

class CallCountChanged: public InvokeBase
{
public:

	CallCountChanged(MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		:InvokeBase(thread_id, tag)
	{
	}

	virtual void Invoke(const std::shared_ptr<const CallInfo>& call_info) const = 0;

};

template<class Delegate>
class CallCountChanged_Imp: public CallCountChanged
{
public:

	CallCountChanged_Imp(Delegate&& del, MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		: CallCountChanged(thread_id, tag)
		, m_del(std::move(del))
	{
	}

	void Invoke(const std::shared_ptr<const CallInfo>& call_info) const override
	{
		m_del(call_info);
	}

	bool IsCallable() const override
	{
		return IsCallable::Convertible(m_del);
	}

private:

	const typename std::decay<Delegate>::type m_del;
};

class CountJob: public MAssIpc_Transthread::Job
{
public:

	CountJob(const std::shared_ptr<const CallCountChanged>& call_count_changed, std::shared_ptr<const CallInfo> call_info)
		:m_call_count_changed(call_count_changed)
		, m_call_info(call_info)
	{
	}

	void Invoke() override
	{
		Invoke(m_call_count_changed, m_call_info);
	}

	static void Invoke(const std::shared_ptr<const CallCountChanged>& call_count_changed, std::shared_ptr<const CallInfo> call_info)
	{
		if( call_count_changed )
			call_count_changed->Invoke(call_info);
	}

private:

	const std::shared_ptr<const CallCountChanged>	m_call_count_changed;
	const std::shared_ptr<const CallInfo>	m_call_info;
};

//-------------------------------------------------------
class ErrorOccured: public InvokeBase
{
public:

	ErrorOccured(MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		:InvokeBase(thread_id, tag)
	{
	}

	virtual void Invoke(ErrorType et, const std::string& message) const = 0;

};

template<class Delegate>
class ErrorOccured_Imp: public ErrorOccured
{
public:

	ErrorOccured_Imp(Delegate&& del, MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		: ErrorOccured(thread_id, tag)
		, m_del(std::move(del))
	{
	}

	void Invoke(ErrorType et, const std::string& message) const override
	{
		m_del(et, message);
	}

	bool IsCallable() const override
	{
		return IsCallable::Convertible(m_del);
	}

private:

	const typename std::decay<Delegate>::type m_del;
};

class ErrorJob: public MAssIpc_Transthread::Job
{
public:

	struct Arg
	{
		ErrorType error;
		std::string message;
	};

	ErrorJob(const std::shared_ptr<const ErrorOccured>& error_occured, const std::shared_ptr<Arg>& arg)
		:m_error_occured(error_occured)
		, m_arg(arg)
	{
	}

	void Invoke() override
	{
		Invoke(m_error_occured, std::move(m_arg));
	}

	static void Invoke(const std::shared_ptr<const ErrorOccured>& error_occured, std::shared_ptr<Arg> arg)
	{
		if( error_occured )
			error_occured->Invoke(arg->error, arg->message);
	}

private:

	const std::shared_ptr<const ErrorOccured>	m_error_occured;
	const std::shared_ptr<Arg> m_arg;
};

//-------------------------------------------------------

class ProcMap
{
public:

	struct CallData
	{
		std::shared_ptr<CallInfoImpl> call_info;
		std::string comment;
	};

	struct FindCallInfoRes
	{
		std::shared_ptr<CallInfoImpl>				call_info;
		std::shared_ptr<InvokeRemoteBase>			invoke_base;
		std::shared_ptr<const CallCountChanged>		on_call_count_changed;
		std::shared_ptr<const ErrorOccured>			on_invalid_remote_call;
	};

	FindCallInfoRes FindCallInfo(const MAssIpc_RawString& proc_name, const MAssIpc_RawString& params_type) const;
	MAssIpcCall_EnumerateData EnumerateHandlers() const;
	std::shared_ptr<const CallInfo> AddProcSignature(const MAssIpc_RawString& proc_name, std::string params_type, std::unique_ptr<InvokeRemoteBase> invoke, const std::string& comment);
	void AddAllProcs(const ProcMap& other);
	void ClearAllProcs();
	void ClearProcsWithTag(const void* tag);

	std::shared_ptr<const CallCountChanged> SetCallCountChanged(std::shared_ptr<const CallCountChanged> new_val);
	std::shared_ptr<const ErrorOccured> SetErrorHandler(std::shared_ptr<const ErrorOccured> new_val);
	std::shared_ptr<const ErrorOccured> GetErrorHandler() const;

private:

	template<class TCallback>
	std::shared_ptr<const TCallback> SetCallback(std::shared_ptr<const TCallback>* member, std::shared_ptr<const TCallback> new_old)
	{
		MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
		member->swap(new_old);
		return new_old;
	}

	template<class TCallback>
	static void ClearCallbackTag(std::shared_ptr<const TCallback>* member, const void* tag)
	{
		if( bool(*member) && ((*member)->m_tag==tag) )
			(*member) = {};
	}

private:


	mutable MAssIpc_ThreadSafe::mutex	m_lock;
	std::map<MAssIpcImpl::CallInfoImpl::SignatureKey, CallData>	m_name_procs;

	std::shared_ptr<const CallCountChanged>		m_OnCallCountChanged;
	std::shared_ptr<const ErrorOccured>			m_OnInvalidRemoteCall;
};


//-------------------------------------------------------

template<size_t...> struct index_tuple
{
};

template<size_t I, typename IndexTuple, typename...Types>
struct make_indexes_impl;

template<size_t I, size_t...Indexes, typename T, typename...Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...>
{
	typedef typename make_indexes_impl<I+1, index_tuple<Indexes..., I>, Types...>::type type;
};

template<size_t I, size_t...Indexes>
struct make_indexes_impl<I, index_tuple<Indexes...> >
{
	typedef index_tuple<Indexes...> type;
};

template<typename...Types>
struct make_indexes: make_indexes_impl<0, index_tuple<>, Types...>
{
};


template<class Func, class Ret, class... Args, size_t...Indexes>
Ret ExpandHelper(Func& pf, index_tuple< Indexes... >, std::tuple<Args...>& tup)
{
	return pf(std::forward<Args>(std::get<Indexes>(tup))...);
}

template<class Func, class Ret, class ... Args>
Ret ExpandTupleCall(Func& pf, std::tuple<Args...>& tup)
{
	return ExpandHelper<Func, Ret>(pf, typename make_indexes<Args...>::type(), tup);
}

//-------------------------------------------------------

template<class...>
using void_t = void;


template <typename TCheckedType, typename = void>
struct IsReadStreamCreating: std::false_type
{
};

template <typename TCheckedType>
struct IsReadStreamCreating<TCheckedType, void_t<decltype(operator>>(std::declval<MAssIpc_DataStream&>(), std::declval<const MAssIpc_DataStream_Create<TCheckedType>&>()))>>: std::true_type
{
};

//-------------------------------------------------------

struct Deserialize_Creating
{
	template<class T>
	static inline T Do(MAssIpc_DataStream& call_info)
	{
		return call_info>>MAssIpc_DataStream_Create<T>();
	}
};

struct Deserialize_DefConstruct
{
	template<class T>
	static inline T Do(MAssIpc_DataStream& call_info)
	{
		T t{};
		call_info>>t;
		return t;
	}
};

template<class T>
static inline T ReadFromStream(MAssIpc_DataStream& call_info)
{
	return std::conditional<IsReadStreamCreating<T>::value, Deserialize_Creating, Deserialize_DefConstruct>::type::template Do<T>(call_info);
}

//-------------------------------------------------------

struct Default_Constructor
{
	template<class T>
	static inline T Do(){return {};}
};

struct Default_FromStream
{
	template<class T>
	static inline T Do()
	{
		static_assert(IsReadStreamCreating<T>::value, "type hase no default constructor and no create from stream operator");
		MAssIpc_DataStream empty_stream;
		return Deserialize_Creating::Do<T>(empty_stream);
	}
};

template<class T>
static inline T MakeDefault()
{
	return std::conditional<std::is_default_constructible<T>::value, Default_Constructor, Default_FromStream>::type::template Do<T>();
}

//-------------------------------------------------------

template<class... Args, size_t... Indexes>
static inline std::tuple<Args...> DeserializeArgs(MAssIpc_DataStream& call_info, index_tuple< Indexes... >)
{
	std::tuple<Args...> tup{(ReadFromStream<typename std::tuple_element<Indexes, std::tuple<Args...>>::type>(call_info))...};
	static_assert(sizeof(tup)>0, "disable warning local variable is initialized but not referenced");
	return tup;
}


template<class... Args>
static inline void SerializeArgs(MAssIpc_DataStream& call_info, const Args&... args)
{
	int unpack[]{0,((call_info<<args),0)...};
	static_assert(sizeof(unpack) > 0, "disable warning local variable is initialized but not referenced");
}

}

//-------------------------------------------------------
										 

MASS_IPC_TYPE_SIGNATURE(void);
MASS_IPC_TYPE_SIGNATURE(int8_t);
MASS_IPC_TYPE_SIGNATURE(uint8_t);
MASS_IPC_TYPE_SIGNATURE(int16_t);
MASS_IPC_TYPE_SIGNATURE(uint16_t);
MASS_IPC_TYPE_SIGNATURE(int32_t);
MASS_IPC_TYPE_SIGNATURE(uint32_t);
MASS_IPC_TYPE_SIGNATURE(int64_t);
MASS_IPC_TYPE_SIGNATURE(uint64_t);
// MASS_IPC_TYPE_SIGNATURE(int);// not use type int because it has different size on different platform
MASS_IPC_TYPE_SIGNATURE(bool);
MASS_IPC_TYPE_SIGNATURE(float);
MASS_IPC_TYPE_SIGNATURE(double);

//-------------------------------------------------------

namespace MAssIpcImpl
{

// template<class T>
// static constexpr void CheckArgType()
// {
// 	static_assert(!std::is_reference<T>::value, "invalid type name contains separator");
// }


static constexpr char separator = ';';

template<const char name_value[]>
static constexpr bool IsSeparatorInName(size_t i)
{
	return (name_value[i]!=separator) && ((name_value[i]==0) || IsSeparatorInName<name_value>(i+1));
}

template<const char t_name[]>
static constexpr bool CheckSeparatorInName()
{
	static_assert(IsSeparatorInName<t_name>(0), "invalid type name contains separator");
	return true;
}


template<class TUnifiedFunction>
struct ProcSignature;

struct ParamsTypeHolder_string
{
	ParamsTypeHolder_string(size_t reserve_size)
	{
		m_value.reserve(reserve_size);
	}

	void AddType(const MAssIpc_RawString& string)
	{
		m_value.append(string.C_String(), string.Length());
		m_value.append(&separator, sizeof(separator));
	}

	std::string m_value;
};


struct ParamsTypeHolder_MAssIpcCallDataStream
{
	void AddType(const MAssIpc_RawString& string)
	{
		m_value.WriteRawData(string.C_String(), string.Length());
		m_value.WriteRawData(&separator, sizeof(separator));
	}

	MAssIpc_DataStream& m_value;
};

struct ParamsTypeHolder_PacketSize
{
	void AddType(const MAssIpc_RawString& string)
	{
		m_size += string.Length();
		m_size += sizeof(separator);
	}

	MAssIpc_Data::PacketSize m_size = 0;
};


template<class Ret, class... Args>
struct ProcSignature<Ret(*)(Args... args)>
{
	template<class ParamsTypeHolder_T>
	static inline void GetParams(ParamsTypeHolder_T* params_type)
	{
		int unpack[]{0,((params_type->AddType(MAssIpc_RawString(MAssIpcType<Args>::NameValue(), MAssIpcType<Args>::NameLength()))
#if !(_MSC_VER==1916)// Visual Studio 2017 compiler crashes trying compile this
						 , CheckSeparatorInName<MAssIpcType<Args>::name_value>()
#endif
						 ),0)...};
		static_assert(sizeof(unpack) > 0, "disable warning local variable is initialized but not referenced");
	};
};

//-------------------------------------------------------

std::unique_ptr<const MAssIpc_Data> SerializeReturn(const std::weak_ptr<MAssIpc_TransportShare>& transport,
														  MAssIpc_PacketParser::CallId respond_id);

template<class Ret>
std::unique_ptr<const MAssIpc_Data> SerializeReturn(const Ret& ret, const std::weak_ptr<MAssIpc_TransportShare>& transport,
												   MAssIpc_PacketParser::CallId respond_id)
{
	if( respond_id == MAssIpc_PacketParser::c_invalid_id )
		return {};
	MAssIpc_DataStream measure_size;
	measure_size<<ret;
	MAssIpc_DataStream result_stream(CreateDataStream(transport, measure_size.GetWritePos(), MAssIpc_PacketParser::PacketType::pt_return, respond_id));
	result_stream<<ret;
	return result_stream.DetachWrite();
}


template<class Delegate, class Signature>
class InvokeRemoteImpl;

template<class Delegate, class Ret, class... Args>
class InvokeRemoteImpl<Delegate, Ret(*)(Args...)>: public InvokeRemoteBase
{
private:
	mutable typename std::decay<Delegate>::type m_del;

private:

	template<bool is_void, typename std::enable_if<!is_void, bool>::type=true>
	std::unique_ptr<const MAssIpc_Data> MakeCallSerializeReturn(const std::weak_ptr<MAssIpc_TransportShare>& transport, MAssIpc_PacketParser::CallId respond_id, std::tuple<Args...>& args) const
	{
		Ret ret=ExpandTupleCall<Delegate, Ret>(m_del, args);
		return SerializeReturn(ret, transport, respond_id);
	}

	template<bool is_void, typename std::enable_if<is_void, bool>::type=true>
	std::unique_ptr<const MAssIpc_Data> MakeCallSerializeReturn(const std::weak_ptr<MAssIpc_TransportShare>& transport, MAssIpc_PacketParser::CallId respond_id, std::tuple<Args...>& args) const
	{
		ExpandTupleCall<Delegate, void>(m_del, args);
		return SerializeReturn(transport, respond_id);
	}


	std::unique_ptr<const MAssIpc_Data> Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, 
										MAssIpc_PacketParser::CallId respond_id,
										MAssIpc_DataStream& params) const override
	{
		std::tuple<Args...> args = DeserializeArgs<Args...>(params, typename make_indexes<Args...>::type());
		return MakeCallSerializeReturn<std::is_same<Ret, void>::value>(transport, respond_id, args);
	}

	bool IsCallable() const override
	{
		return IsCallable::Convertible(m_del);
	}

	MAssIpc_RawString GetSignature_ReturnType() const override
	{
		return {MAssIpcType<Ret>::NameValue(), MAssIpcType<Ret>::NameLength()};
	}

public:

	InvokeRemoteImpl(Delegate&& del, MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
		:InvokeRemoteBase(thread_id, tag)
		, m_del(std::move(del))
	{
	};
};


template<class Delegate>
struct Impl_Selector
{
	using Res=InvokeRemoteImpl<Delegate, typename FuncSig<Delegate>::FuncPtr>;
};

//-------------------------------------------------------

template<class Delegate>
std::string CallInfoImpl::MakeParamsType()
{
	MAssIpcImpl::ParamsTypeHolder_PacketSize params_type_calc;
	using FuncPtr = typename MAssIpcImpl::FuncSig<Delegate>::FuncPtr;
	MAssIpcImpl::ProcSignature<FuncPtr>::GetParams(&params_type_calc);
	MAssIpcImpl::ParamsTypeHolder_string params_type_string(params_type_calc.m_size);
	MAssIpcImpl::ProcSignature<FuncPtr>::GetParams(&params_type_string);
	return std::move(params_type_string.m_value);
}

//-------------------------------------------------------


class MAssIpcData_Vector: public MAssIpc_Data
{
public:

	MAssIpcData_Vector(MAssIpc_Data::PacketSize size)
		: m_storage(new uint8_t[size])
		, m_storage_size(size)
	{
	}

	MAssIpc_Data::PacketSize Size() const override
	{
		return m_storage_size;
	}

	uint8_t* Data() override
	{
		return m_storage.get();
	}

	const uint8_t* Data() const override
	{
		return m_storage.get();
	}

private:

	std::unique_ptr<uint8_t[]> m_storage;
	const MAssIpc_Data::PacketSize m_storage_size;
};


}// namespace MAssIpcImpl;


