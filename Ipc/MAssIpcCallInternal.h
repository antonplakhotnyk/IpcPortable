#pragma once

#include <map>
#include "MAssIpcCallDataStream.h"
#include "MAssCallThreadTransport.h"
#include <functional>
#include "MAssIpcPacketParser.h"
#include "MAssIpcCallTransport.h"
#include "MAssIpcThreadSafe.h"
#include "MAssIpcRawString.h"

namespace MAssIpcCallInternal
{

MAssIpcCallDataStream CreateDataStream(const std::weak_ptr<MAssIpcPacketTransport>& weak_transport,
									   MAssIpcData::TPacketSize no_header_size,
									   MAssIpcPacketParser::PacketType pt,
									   MAssIpcPacketParser::TCallId respond_id);

MAssIpcCallDataStream CreateDataStreamInplace(std::unique_ptr<MAssIpcData> inplace_send_buffer,
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


	CallInfo(MAssIpcThreadTransportTarget::Id thread_id, const MAssIpcRawString& proc_name, std::string params_type);

	virtual std::unique_ptr<const MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
												MAssIpcPacketParser::TCallId respond_id,
												MAssIpcCallDataStream& params) const = 0;
	virtual bool IsCallable() const = 0;
	virtual MAssIpcRawString GetSignature_RetType() const = 0;

	struct SignatureKey
	{
		MAssIpcRawString name;
		MAssIpcRawString params_type;

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

public:

	const MAssIpcThreadTransportTarget::Id m_thread_id;

protected:

	const std::string m_name;
	const std::string m_params_type;
};


class ResultJob: public MAssCallThreadTransport::Job
{
public:

	ResultJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<const MAssIpcData>& result);

	void Invoke() override;
	static void Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<const MAssIpcData>& result);

private:

	std::weak_ptr<MAssIpcPacketTransport> m_transport;
	std::unique_ptr<const MAssIpcData> m_result;
};


class CallJob: public MAssCallThreadTransport::Job
{
public:

	CallJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
			const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
			MAssIpcCallDataStream& call_info_data, MAssIpcPacketParser::TCallId respond_id,
			std::shared_ptr<const CallInfo> call_info);
	void Invoke() override;
	static void Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
					   MAssIpcCallDataStream& call_info_data, std::shared_ptr<const CallInfo> call_info, 
					   MAssIpcPacketParser::TCallId respond_id);
	static MAssIpcPacketParser::TCallId CalcRespondId(bool send_return, MAssIpcPacketParser::TCallId respond_id);

	MAssIpcCallDataStream					m_call_info_data;
	const std::shared_ptr<const CallInfo>	m_call_info;
	const MAssIpcPacketParser::TCallId		m_respond_id;

private:

	const std::weak_ptr<MAssIpcPacketTransport>	m_transport;
	const std::weak_ptr<MAssCallThreadTransport>	m_inter_thread;
};


class ProcMap
{
public:

	std::shared_ptr<const CallInfo> FindCallInfo(const MAssIpcRawString& name, const MAssIpcRawString& params_type) const;
	MAssIpcCall_EnumerateData EnumerateHandlers() const;
	void AddProcSignature(const std::shared_ptr<MAssIpcCallInternal::CallInfo>& call_info, const std::string& comment);
	void AddAllProcs(const ProcMap& other);
	void ClearAllProcs();

public:

	struct CallData
	{
		std::shared_ptr<CallInfo> call_info;
		std::string comment;
	};

private:


	mutable MAssIpcThreadSafe::mutex	m_lock;
	std::map<MAssIpcCallInternal::CallInfo::SignatureKey, CallData>	m_name_procs;
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

struct ParamsTypeHolder_string
{
	ParamsTypeHolder_string(size_t reserve_size)
	{
		m_value.reserve(reserve_size);
	}

	void AddType(const MAssIpcRawString& string)
	{
		m_value.append(string.C_String(), string.Length());
		m_value.append(&separator, sizeof(separator));
	}

	std::string m_value;
};


struct ParamsTypeHolder_MAssIpcCallDataStream
{
	void AddType(const MAssIpcRawString& string)
	{
		m_value.WriteRawData(string.C_String(), string.Length());
		m_value.WriteRawData(&separator, sizeof(separator));
	}

	MAssIpcCallDataStream& m_value;
};

struct ParamsTypeHolder_TPacketSize
{
	void AddType(const MAssIpcRawString& string)
	{
		m_size += string.Length();
		m_size += sizeof(separator);
	}

	MAssIpcData::TPacketSize m_size = 0;
};


template<class TRet, class... TArgs>
struct ProcSignature<TRet(*)(TArgs... args)>
{
	template<class ParamsTypeHolder_T>
	static inline void GetParams(ParamsTypeHolder_T* params_type)
	{
		int unpack[]{0,((params_type->AddType(MAssIpcRawString(MAssIpcType<TArgs>::NameValue(), MAssIpcType<TArgs>::NameLength()))
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
static inline bool IsBoolConvertible_Callable(const TDelegate& del)
{
	return std::conditional<IsConvertible<TDelegate, bool>::value, IsCallable_Check, IsCallable_True>::type::ToBool(del);
}

//-------------------------------------------------------

std::unique_ptr<const MAssIpcData> SerializeReturn(const std::weak_ptr<MAssIpcPacketTransport>& transport,
														  MAssIpcPacketParser::TCallId respond_id);

template<class TRet>
std::unique_ptr<const MAssIpcData> SerializeReturn(const TRet& ret, const std::weak_ptr<MAssIpcPacketTransport>& transport,
												   MAssIpcPacketParser::TCallId respond_id)
{
	if( respond_id == MAssIpcPacketParser::c_invalid_id )
		return {};
	MAssIpcCallDataStream measure_size;
	measure_size<<ret;
	MAssIpcCallDataStream result_stream(CreateDataStream(transport, measure_size.GetWritePos(), MAssIpcPacketParser::PacketType::pt_return, respond_id));
	result_stream<<ret;
	return result_stream.DetachWrite();
}

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

		std::unique_ptr<const MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
											MAssIpcPacketParser::TCallId respond_id,
											MAssIpcCallDataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			ExpandTupleCall<TDelegate, void>(m_del, args);

			return SerializeReturn(transport, respond_id);
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible_Callable(m_del);
		}

		MAssIpcRawString GetSignature_RetType() const override
		{
			return {MAssIpcType<void>::NameValue(), MAssIpcType<void>::NameLength()};
		}

	public:

		Imp(const TDelegate& del, MAssIpcThreadTransportTarget::Id thread_id, const MAssIpcRawString& name)
			:CallInfo(thread_id, name, CallInfo::MakeParamsType<TDelegate>() ), m_del(del)
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

		std::unique_ptr<const MAssIpcData> Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
											MAssIpcPacketParser::TCallId respond_id,
											MAssIpcCallDataStream& params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			TRet ret = ExpandTupleCall<TDelegate, TRet>(m_del, args);

			return SerializeReturn(ret, transport, respond_id);
		}

		bool IsCallable() const override
		{
			return IsBoolConvertible_Callable(m_del);
		}

		MAssIpcRawString GetSignature_RetType() const override
		{
			return {MAssIpcType<TRet>::NameValue(), MAssIpcType<TRet>::NameLength()};
		}

	public:

		Imp(const TDelegate& del, MAssIpcThreadTransportTarget::Id thread_id, const MAssIpcRawString& name)
			:CallInfo(thread_id, name, CallInfo::MakeParamsType<TDelegate>()), m_del(del)
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
std::string CallInfo::MakeParamsType()
{
	MAssIpcCallInternal::ParamsTypeHolder_TPacketSize params_type_calc;
	using TreatProc = typename MAssIpcCallInternal::Impl_Selector<TDelegate>::TDelProcUnified;
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_calc);
	MAssIpcCallInternal::ParamsTypeHolder_string params_type_string(params_type_calc.m_size);
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_string);
	return std::move(params_type_string.m_value);
}


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

	const uint8_t* Data() const override
	{
		return m_storage.get();
	}

private:

	std::unique_ptr<uint8_t[]> m_storage;
	const MAssIpcData::TPacketSize m_storage_size;
};


}// namespace MAssIpcCallInternal;


