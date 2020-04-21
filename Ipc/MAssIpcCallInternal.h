#pragma once

#include <map>
#include "MAssIpcCallDataStream.h"
#include "MAssCallThreadTransport.h"
#include <stdint.h>
#include <functional>
#include "MAssIpcCallPacket.h"
#include "MAssIpcCallTransport.h"

namespace MAssIpcCallInternal
{

//-------------------------------------------------------

class CallInfo
{
public:

	CallInfo(MAssThread::Id thread_id):m_thread_id(thread_id){};

	virtual void Invoke(MAssIpcCallDataStream* res, MAssIpcCallDataStream* params) const = 0;
	virtual bool IsCallable() const = 0;
	virtual const char* GetSignature_RetType() const = 0;
	MAssThread::Id ThreadId() const
	{
		return m_thread_id;
	}

private:

	const MAssThread::Id m_thread_id;
};


class ResultJob: public MAssCallThreadTransport::Job
{
public:

	ResultJob(const std::shared_ptr<MAssIpcCallTransport>& transport)
		:m_transport(transport)
	{
	};

	void Invoke() override
	{
		MAssIpcCallPacket::SendData(m_result, MAssIpcCallPacket::pt_return, m_transport);
	}

	std::vector<uint8_t> m_result;

private:

	std::shared_ptr<MAssIpcCallTransport> m_transport;
};


class CallJob: public MAssCallThreadTransport::Job
{
public:

	CallJob(const std::shared_ptr<MAssIpcCallTransport>& transport, const std::shared_ptr<MAssCallThreadTransport>& inter_thread,
			std::unique_ptr<std::vector<uint8_t> > call_info_data)
		: m_call_info_data_str(call_info_data->data(), call_info_data->size())
		, m_call_info_data(std::move(call_info_data))
		, m_result_thread_id(inter_thread ? inter_thread->GetCurrentThreadId() : MAssThread::c_no_thread)
		, m_transport(transport)
		, m_inter_thread(inter_thread)
	{
	}

	void Invoke() override
	{
		std::shared_ptr<ResultJob> result_job(new ResultJob(m_transport));
		MAssIpcCallDataStream result_str(&result_job->m_result);
		m_call_info->Invoke(&result_str, &m_call_info_data_str);
		if( m_send_return )
		{
			if( m_inter_thread )
				m_inter_thread->CallFromThread(m_result_thread_id, result_job);
			else
				result_job->Invoke();
		}
	}

	MAssIpcCallDataStream				m_call_info_data_str;
	std::shared_ptr<const CallInfo>	m_call_info;
	bool							m_send_return=false;

private:

	std::unique_ptr<const std::vector<uint8_t> >		m_call_info_data;
	MAssThread::Id			m_result_thread_id;
	std::shared_ptr<MAssIpcCallTransport>					m_transport;
	std::shared_ptr<MAssCallThreadTransport>				m_inter_thread;
};


class ProcMap
{
public:

	void FindCallInfo(const std::string& name, std::string& signature, std::shared_ptr<const CallInfo>* call_info) const
	{
		auto it_procs = m_name_procs.find(name);
		if( it_procs==m_name_procs.end() )
			return;

		auto it_signature = it_procs->second.m_signature_call.find(signature);
		if( it_signature==it_procs->second.m_signature_call.end() )
			return;

		*call_info = it_signature->second.call_info;
	}

	MAssIpcCall_EnumerateData EnumerateHandlers()
	{
		MAssIpcCall_EnumerateData res;

		for( auto it_np = m_name_procs.begin(); it_np!=m_name_procs.end(); it_np++ )
			for( auto it_sc = it_np->second.m_signature_call.begin(); it_sc!=it_np->second.m_signature_call.end(); it_sc++ )
				res.push_back({it_np->first, it_sc->second.call_info->GetSignature_RetType(), it_sc->first, it_sc->second.comment});

		return res;
	}

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

	std::map<std::string, NameProcs> m_name_procs;
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


template<class Ret, class... Args, size_t... Indexes >
Ret ExpandHelper(const std::function<Ret(Args...)>& pf, index_tuple< Indexes... >, std::tuple<Args...>& tup)
{
	return pf(std::forward<Args>(std::get<Indexes>(tup))...);
}

template<class Ret, class ... Args>
Ret ExpandTupleCall(const std::function<Ret(Args...)>& pf, std::tuple<Args...>& tup)
{
	return ExpandHelper(pf, typename make_indexes<Args...>::type(), tup);
}

//-------------------------------------------------------

template<class... TArgs, size_t... Indexes>
static inline std::tuple<TArgs...> DeserializeArgs(MAssIpcCallDataStream* call_info, index_tuple< Indexes... >)
{
	std::tuple<TArgs...> tup;
	int unpack[]{0,(((*call_info)>>std::get<Indexes>(tup)),0)...};
	unpack;
	return tup;
}


template<class... TArgs>
static inline void SerializeArgs(MAssIpcCallDataStream* call_info, TArgs&... args)
{
	int unpack[]{0,(((*call_info)<<args),0)...};
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
static constexpr void CheckSeparatorInName()
{
	static_assert(IsSeparatorInName<t_name>(0), "invalid type name contains separator");
}


template<class TUnifiedFunction>
struct ProcSignature;

template<class TRet, class... TArgs>
struct ProcSignature<TRet(*)(TArgs... args)>
{
	static inline void GetParams(std::string* params_type)
	{
		int unpack[]{0,((*params_type += MAssIpcType<TArgs>::NameValue(), *params_type += separator, CheckSeparatorInName<MAssIpcType<TArgs>::name_value>() ),0)...};
		unpack;
	};

};

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

		void Invoke(MAssIpcCallDataStream* res, MAssIpcCallDataStream* params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			ExpandTupleCall(m_del, args);
		}

		bool IsCallable() const override
		{
			return bool(m_del);
		}

		const char* GetSignature_RetType() const override
		{
			return MAssIpcType<void>::NameValue();
		}

	public:

		Imp(const TDelegate& del, MAssThread::Id thread_id): CallInfo(thread_id), m_del(del)
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

		void Invoke(MAssIpcCallDataStream* res, MAssIpcCallDataStream* params) const override
		{
			std::tuple<TArgs...> args = DeserializeArgs<TArgs...>(params, typename make_indexes<TArgs...>::type());
			TRet ret = ExpandTupleCall(m_del, args);
			if( res )
				(*res)<<ret;
		}

		bool IsCallable() const override
		{
			return bool(m_del);
		}

		const char* GetSignature_RetType() const override
		{
			return MAssIpcType<TRet>::NameValue();
		}

	public:

		Imp(const TDelegate& del, MAssThread::Id thread_id):CallInfo(thread_id), m_del(del)
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



}// namespace MAssIpcCallInternal;


