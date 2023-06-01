#pragma once

#include <map>
#include "MAssIpc_DataStream.h"
#include "MAssIpc_Transthread.h"
#include <functional>
#include "MAssIpc_PacketParser.h"
#include "MAssIpc_Transport.h"
#include "MAssIpc_ThreadSafe.h"
#include "MAssIpc_RawString.h"

namespace MAssIpcCallInternal
{

MAssIpc_DataStream CreateDataStream(const std::weak_ptr<MAssIpc_TransportShare>& weak_transport,
									   MAssIpc_Data::TPacketSize no_header_size,
									   MAssIpc_PacketParser::PacketType pt,
									   MAssIpc_PacketParser::TCallId respond_id);

MAssIpc_DataStream CreateDataStreamInplace(std::unique_ptr<MAssIpc_Data> inplace_send_buffer,
											  MAssIpc_Data::TPacketSize no_header_size,
											  MAssIpc_PacketParser::PacketType pt,
											  MAssIpc_PacketParser::TCallId respond_id);

//-------------------------------------------------------

template<class TFrom, class TTo>
auto DeduceAvailable(int) -> decltype(TTo(std::declval<TFrom>()), std::true_type{});

template<class, class>
auto DeduceAvailable(...)->std::false_type;

template<class TFrom, class TTo>
class IsConvertible: public decltype(DeduceAvailable<TFrom, TTo>(0))
{
};

//-------------------------------------------------------


class CallInfo
{
public:
	virtual ~CallInfo() = default;
	virtual uint32_t GetCallCount() const = 0;
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


using TErrorHandler = std::function<void(ErrorType et, std::string message)>;
using TCallCountChanged = std::function<void(std::shared_ptr<const CallInfo> call_info)>;

class CallInfoImpl: public CallInfo
{
public:


	CallInfoImpl(MAssIpc_TransthreadTarget::Id thread_id, const MAssIpc_RawString& proc_name, std::string params_type);

	virtual std::unique_ptr<const MAssIpc_Data> Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, 
												MAssIpc_PacketParser::TCallId respond_id,
												MAssIpc_DataStream& params) const = 0;
	virtual bool IsCallable() const = 0;
	virtual MAssIpc_RawString GetSignature_RetType() const = 0;

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

	template<class TDelegate>
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

public:

	const MAssIpc_TransthreadTarget::Id m_thread_id;

protected:

	MAssIpc_ThreadSafe::atomic_uint32_t	m_call_count = {0};
	const std::string		m_name;
	const std::string		m_params_type;
};


class ResultJob: public MAssIpc_Transthread::Job
{
public:

	ResultJob(const std::weak_ptr<MAssIpc_TransportShare>& transport, std::unique_ptr<const MAssIpc_Data>& result);

	void Invoke() override;
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
			const std::shared_ptr<const TCallCountChanged>& call_count_changed,
			MAssIpc_DataStream& call_info_data, MAssIpc_PacketParser::TCallId respond_id,
			std::shared_ptr<const CallInfoImpl> call_info);
	void Invoke() override;
	static void Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread,
					   const std::shared_ptr<const TCallCountChanged>& call_count_changed,
					   MAssIpc_DataStream& call_info_data, std::shared_ptr<const CallInfoImpl> call_info, 
					   MAssIpc_PacketParser::TCallId respond_id);
	static MAssIpc_PacketParser::TCallId CalcRespondId(bool send_return, MAssIpc_PacketParser::TCallId respond_id);

	const std::shared_ptr<const TCallCountChanged>	m_call_count_changed;
	MAssIpc_DataStream					m_call_info_data;
	const std::shared_ptr<const CallInfoImpl>	m_call_info;
	const MAssIpc_PacketParser::TCallId		m_respond_id;

private:

	const std::weak_ptr<MAssIpc_TransportShare>	m_transport;
	const std::weak_ptr<MAssIpc_Transthread>	m_inter_thread;
};


class ProcMap
{
public:

	struct CallData
	{
		std::shared_ptr<CallInfoImpl> call_info;
		std::string comment;
		const void* const	tag = nullptr;
	};

	struct FindCallInfoRes
	{
		std::shared_ptr<CallInfoImpl>				call_info;
		std::shared_ptr<const TCallCountChanged>	on_call_count_changed;
		std::shared_ptr<const TErrorHandler>		on_invalid_remote_call;
	};

	FindCallInfoRes FindCallInfo(const MAssIpc_RawString& name, const MAssIpc_RawString& params_type) const;
	MAssIpcCall_EnumerateData EnumerateHandlers() const;
	std::shared_ptr<const CallInfo> AddProcSignature(const std::shared_ptr<MAssIpcCallInternal::CallInfoImpl>& call_info, const std::string& comment, const void* tag);
	void AddAllProcs(const ProcMap& other);
	void ClearAllProcs();
	void ClearProcsWithTag(const void* tag);

	std::shared_ptr<const TCallCountChanged> SetCallCountChanged(std::shared_ptr<const TCallCountChanged> new_val);
	std::shared_ptr<const TErrorHandler> SetErrorHandler(std::shared_ptr<const TErrorHandler> new_val);
	std::shared_ptr<const TErrorHandler> GetErrorHandler() const;

private:

	template<class TCallback>
	std::shared_ptr<const TCallback> SetCallback(std::shared_ptr<const TCallback>* member, std::shared_ptr<const TCallback> new_old)
	{
		MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
		member->swap(new_old);
		return new_old;
	}

private:


	mutable MAssIpc_ThreadSafe::mutex	m_lock;
	std::map<MAssIpcCallInternal::CallInfoImpl::SignatureKey, CallData>	m_name_procs;

	std::shared_ptr<const TCallCountChanged>		m_OnCallCountChanged = std::make_shared<const TCallCountChanged>();
	std::shared_ptr<const TErrorHandler>			m_OnInvalidRemoteCall = std::make_shared<const TErrorHandler>();
};


template<class TDelProc>
class Sig_RetVoidNo;

template<class TDelProc>
class Sig_RetVoid;

template<class TFunction>
struct InfoDelproc;

template<class TRet, class TCls, class ...Args>
struct InfoDelproc<TRet(TCls::*)(Args...) const>
{
	typedef TRet TRetType;

	typedef TRet(*THandProcUnifiedType_RetType)(Args...);
	typedef void(*THandProcUnifiedType_RetVoid)(Args...);
};


//-------------------------------------------------------

template<size_t...> struct index_tuple
{
};

template<size_t I, typename IndexTuple, typename... Types>
struct make_indexes_impl;

template<size_t I, size_t... Indexes, typename T, typename ... Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...>
{
	typedef typename make_indexes_impl<I + 1, index_tuple<Indexes..., I>, Types...>::type type;
};

template<size_t I, size_t... Indexes>
struct make_indexes_impl<I, index_tuple<Indexes...> >
{
	typedef index_tuple<Indexes...> type;
};

template<typename ... Types>
struct make_indexes: make_indexes_impl<0, index_tuple<>, Types...>
{
};


template<class TFunc, class Ret, class... Args, size_t... Indexes >
Ret ExpandHelper(const TFunc& pf, index_tuple< Indexes... >, std::tuple<Args...>& tup)
{
	return pf(std::forward<Args>(std::get<Indexes>(tup))...);
}

template<class TFunc, class Ret, class ... Args>
Ret ExpandTupleCall(const TFunc& pf, std::tuple<Args...>& tup)
{
	return ExpandHelper<TFunc, Ret>(pf, typename make_indexes<Args...>::type(), tup);
}

//-------------------------------------------------------

template<class... TArgs, size_t... Indexes>
static inline std::tuple<TArgs...> DeserializeArgs(MAssIpc_DataStream& call_info, index_tuple< Indexes... >)
{
	std::tuple<TArgs...> tup;
	int unpack[]{0,((call_info>>std::get<Indexes>(tup)),0)...};
	static_assert(sizeof(unpack)>0, "disable warning local variable is initialized but not referenced");
	return tup;
}


template<class... TArgs>
static inline void SerializeArgs(MAssIpc_DataStream& call_info, const TArgs&... args)
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

namespace MAssIpcCallInternal
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

struct ParamsTypeHolder_TPacketSize
{
	void AddType(const MAssIpc_RawString& string)
	{
		m_size += string.Length();
		m_size += sizeof(separator);
	}

	MAssIpc_Data::TPacketSize m_size = 0;
};


template<class TRet, class... TArgs>
struct ProcSignature<TRet(*)(TArgs... args)>
{
	template<class ParamsTypeHolder_T>
	static inline void GetParams(ParamsTypeHolder_T* params_type)
	{
		int unpack[]{0,((params_type->AddType(MAssIpc_RawString(MAssIpcType<TArgs>::NameValue(), MAssIpcType<TArgs>::NameLength()))
#if !(_MSC_VER==1916)// Visual Studio 2017 compiler crashes trying compile this
						 , CheckSeparatorInName<MAssIpcType<TArgs>::name_value>()
#endif
						 ),0)...};
		static_assert(sizeof(unpack) > 0, "disable warning local variable is initialized but not referenced");
	};
};

struct IsCallable_Check
{
	template<class TDelegate>
	static inline bool ToBool(const TDelegate& del){return bool(del);}
};

struct IsCallable_True
{
	template<class TDelegate>
	static constexpr bool ToBool(const TDelegate& del){return true;}
};

template<class TDelegate>
static inline bool IsBoolConvertible_Callable(const TDelegate& del)
{
	return std::conditional<IsConvertible<TDelegate, bool>::value, IsCallable_Check, IsCallable_True>::type::ToBool(del);
}

//-------------------------------------------------------

std::unique_ptr<const MAssIpc_Data> SerializeReturn(const std::weak_ptr<MAssIpc_TransportShare>& transport,
														  MAssIpc_PacketParser::TCallId respond_id);

template<class TRet>
std::unique_ptr<const MAssIpc_Data> SerializeReturn(const TRet& ret, const std::weak_ptr<MAssIpc_TransportShare>& transport,
												   MAssIpc_PacketParser::TCallId respond_id)
{
	if( respond_id == MAssIpc_PacketParser::c_invalid_id )
		return {};
	MAssIpc_DataStream measure_size;
	measure_size<<ret;
	MAssIpc_DataStream result_stream(CreateDataStream(transport, measure_size.GetWritePos(), MAssIpc_PacketParser::PacketType::pt_return, respond_id));
	result_stream<<ret;
	return result_stream.DetachWrite();
}

template<class... TArgs>
class Sig_RetVoid<void(*)(TArgs...)>
{
public:

	template<class TDelegate>
	class Imp: public CallInfoImpl
	{
	private:

		TDelegate m_del;

	private:

		std::unique_ptr<const MAssIpc_Data> Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, 
											MAssIpc_PacketParser::TCallId respond_id,
											MAssIpc_DataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			ExpandTupleCall<TDelegate, void>(m_del, args);

			return SerializeReturn(transport, respond_id);
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible_Callable(m_del);
		}

		MAssIpc_RawString GetSignature_RetType() const override
		{
			return {MAssIpcType<void>::NameValue(), MAssIpcType<void>::NameLength()};
		}

	public:

		Imp(const TDelegate& del, MAssIpc_TransthreadTarget::Id thread_id, const MAssIpc_RawString& name)
			:CallInfoImpl(thread_id, name, CallInfoImpl::MakeParamsType<TDelegate>() ), m_del(del)
		{
		};
	};
};

template<class TRet, class... TArgs>
class Sig_RetVoidNo<TRet(*)(TArgs...)>
{
public:

	template<class TDelegate>
	class Imp: public CallInfoImpl
	{
	private:

		TDelegate m_del;

	private:

		std::unique_ptr<const MAssIpc_Data> Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, 
											MAssIpc_PacketParser::TCallId respond_id,
											MAssIpc_DataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			TRet ret = ExpandTupleCall<TDelegate, TRet>(m_del, args);

			return SerializeReturn(ret, transport, respond_id);
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible_Callable(m_del);
		}

		MAssIpc_RawString GetSignature_RetType() const override
		{
			return {MAssIpcType<TRet>::NameValue(), MAssIpcType<TRet>::NameLength()};
		}

	public:

		Imp(const TDelegate& del, MAssIpc_TransthreadTarget::Id thread_id, const MAssIpc_RawString& name)
			:CallInfoImpl(thread_id, name, CallInfoImpl::MakeParamsType<TDelegate>()), m_del(del)
		{
		};
	};

};

template<class TDelegate>
class Impl_Selector
{
	typedef decltype(&TDelegate::operator()) TDelProc;
public:

	typedef typename InfoDelproc<TDelProc>::THandProcUnifiedType_RetType TDelProcUnified;

	typedef typename
		std::conditional<std::is_same<typename InfoDelproc<TDelProc>::TRetType,void>::value,

		Sig_RetVoid<typename InfoDelproc<TDelProc>::THandProcUnifiedType_RetType>,
		Sig_RetVoidNo<typename InfoDelproc<TDelProc>::THandProcUnifiedType_RetType> >::type

		::template Imp<TDelegate> Res;
};

//-------------------------------------------------------

template<class TDelegate>
std::string CallInfoImpl::MakeParamsType()
{
	MAssIpcCallInternal::ParamsTypeHolder_TPacketSize params_type_calc;
	using TreatProc = typename MAssIpcCallInternal::Impl_Selector<TDelegate>::TDelProcUnified;
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_calc);
	MAssIpcCallInternal::ParamsTypeHolder_string params_type_string(params_type_calc.m_size);
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_string);
	return std::move(params_type_string.m_value);
}


//-------------------------------------------------------


class MAssIpcData_Vector: public MAssIpc_Data
{
public:

	MAssIpcData_Vector(MAssIpc_Data::TPacketSize size)
		: m_storage(new uint8_t[size])
		, m_storage_size(size)
	{
	}

	MAssIpc_Data::TPacketSize Size() const override
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
	const MAssIpc_Data::TPacketSize m_storage_size;
};


}// namespace MAssIpcCallInternal;


