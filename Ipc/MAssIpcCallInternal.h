#pragma once

#include <map>
#include "MAssIpcCallDataStream.h"
#include "MAssCallThreadTransport.h"
#include <functional>
#include "MAssIpcPacketParser.h"
#include "MAssIpcCallTransport.h"
#include "MAssIpcThreadSafe.h"


namespace MAssIpcCallInternal
{

MAssIpcCallDataStream CreateDataStream(const std::weak_ptr<MAssIpcPacketTransport>& weak_transport,
									   MAssIpcData::TPacketSize no_header_size,
									   MAssIpcPacketParser::PacketType pt,
									   MAssIpcPacketParser::TCallId respond_id);

MAssIpcCallDataStream CreateDataStreamInplace(std::unique_ptr<MAssIpcData>& inplace_send_buffer,
											  MAssIpcData::TPacketSize no_header_size,
											  MAssIpcPacketParser::PacketType pt,
											  MAssIpcPacketParser::TCallId respond_id);

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

	CallInfo(MAssIpcThreadTransportTarget::Id thread_id);

	virtual std::unique_ptr<MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
												MAssIpcPacketParser::TCallId respond_id,
												MAssIpcCallDataStream& params) const = 0;
	virtual bool IsCallable() const = 0;
	virtual const char* GetSignature_RetType() const = 0;

public:

	const MAssIpcThreadTransportTarget::Id m_thread_id;
};


class ResultJob: public MAssCallThreadTransport::Job
{
public:

	ResultJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<MAssIpcData>& result);

	void Invoke() override;

private:

	std::unique_ptr<MAssIpcData> m_result;
	std::weak_ptr<MAssIpcPacketTransport> m_transport;
};


class CallJob: public MAssCallThreadTransport::Job
{
public:

	CallJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
			MAssIpcCallDataStream& call_info_data, MAssIpcPacketParser::TCallId id);
	void Invoke() override;

	MAssIpcCallDataStream			m_call_info_data_str;
	std::shared_ptr<const CallInfo>	m_call_info;
	MAssIpcPacketParser::TCallId	m_id;
	bool							m_send_return=false;

private:

	static MAssIpcThreadTransportTarget::Id MakeResultThreadId(const std::weak_ptr<MAssCallThreadTransport>& inter_thread);

private:

	MAssIpcThreadTransportTarget::Id			m_result_thread_id;
	std::weak_ptr<MAssIpcPacketTransport>				m_transport;
	std::weak_ptr<MAssCallThreadTransport>				m_inter_thread;
};


class ProcMap
{
public:

	std::shared_ptr<const CallInfo> FindCallInfo(const std::string& name, std::string& signature) const;
	MAssIpcCall_EnumerateData EnumerateHandlers() const;
	void AddProcSignature(const std::string& proc_name, std::string& params_type, const std::shared_ptr<MAssIpcCallInternal::CallInfo>& call_info, const std::string& comment);
	void AddAllProcs(const ProcMap& other);
	void ClearAllProcs();

public:

	struct CallData
	{
		std::shared_ptr<CallInfo> call_info;
		std::string comment;
	};

	struct NameProcs
	{
		std::map<std::string, CallData> m_signature_call;
	};

private:


	mutable MAssIpcThreadSafe::mutex	m_lock;
	std::map<std::string, NameProcs>	m_name_procs;
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
static inline std::tuple<TArgs...> DeserializeArgs(MAssIpcCallDataStream& call_info, index_tuple< Indexes... >)
{
	std::tuple<TArgs...> tup;
	int unpack[]{0,((call_info>>std::get<Indexes>(tup)),0)...};
	unpack;
	return tup;
}


template<class... TArgs>
static inline void SerializeArgs(MAssIpcCallDataStream& call_info, const TArgs&... args)
{
	int unpack[]{0,((call_info<<args),0)...};
	unpack;
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

template<class TRet, class... TArgs>
struct ProcSignature<TRet(*)(TArgs... args)>
{
	static inline void GetParams(std::string* params_type)
	{
		int unpack[]{0,((*params_type += MAssIpcType<TArgs>::NameValue(), *params_type += separator
#if !(_MSC_VER==1916)// Visual Studio 2017 compiler crashes trying compile this
						 , CheckSeparatorInName<MAssIpcType<TArgs>::name_value>() 
#endif

						 ),0)...};
		unpack;
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
static inline bool IsBoolConvertible(const TDelegate& del)
{
	return std::conditional<IsConvertible<TDelegate, bool>::value, IsCallable_Check, IsCallable_True>::type::ToBool(del);
}

//-------------------------------------------------------

template<class... TArgs>
class Sig_RetVoid<void(*)(TArgs...)>
{
public:

	template<class TDelegate>
	class Imp: public CallInfo
	{
	private:

		TDelegate m_del;

	private:

		std::unique_ptr<MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
											MAssIpcPacketParser::TCallId respond_id,
											MAssIpcCallDataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			ExpandTupleCall<TDelegate, void>(m_del, args);

			MAssIpcCallDataStream data_stream(CreateDataStream(transport, 0, MAssIpcPacketParser::PacketType::pt_return, respond_id));
			return data_stream.DetachData();
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible(m_del);
		}

		const char* GetSignature_RetType() const override
		{
			return MAssIpcType<void>::NameValue();
		}

	public:

		Imp(const TDelegate& del, MAssIpcThreadTransportTarget::Id thread_id): CallInfo(thread_id), m_del(del)
		{
		};
	};
};



template<class TRet, class... TArgs>
class Sig_RetVoidNo<TRet(*)(TArgs...)>
{
public:

	template<class TDelegate>
	class Imp: public CallInfo
	{
	private:

		TDelegate m_del;

	private:

		std::unique_ptr<MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
											MAssIpcPacketParser::TCallId respond_id,
											MAssIpcCallDataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			TRet ret = ExpandTupleCall<TDelegate, TRet>(m_del, args);

			MAssIpcCallDataStream measure_size;
			measure_size<<ret;

			MAssIpcCallDataStream result_str = CreateDataStream(transport, measure_size.GetWritePos(), MAssIpcPacketParser::PacketType::pt_return, respond_id);
			result_str<<ret;
			return result_str.DetachData();
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible(m_del);
		}

		const char* GetSignature_RetType() const override
		{
			return MAssIpcType<TRet>::NameValue();
		}

	public:

		Imp(const TDelegate& del, MAssIpcThreadTransportTarget::Id thread_id):CallInfo(thread_id), m_del(del)
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

class MAssIpcData_Vector: public MAssIpcData
{
public:

	MAssIpcData_Vector() = default;
	MAssIpcData_Vector(MAssIpcData::TPacketSize size)
		: m_storage(std::make_unique<uint8_t[]>(size))
		, m_storage_size(size)
	{
	}

	MAssIpcData::TPacketSize Size() const override
	{
		return m_storage_size;
	}

	uint8_t* Data() override
	{
		return m_storage.get();
	}

private:

	std::unique_ptr<uint8_t[]> m_storage;
	const MAssIpcData::TPacketSize m_storage_size;
};


}// namespace MAssIpcCallInternal;


