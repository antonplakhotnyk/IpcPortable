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

	CallInfo(MAssThread::Id thread_id);

	virtual void Invoke(MAssIpcCallDataStream* res, MAssIpcCallDataStream* params) const = 0;
	virtual bool IsCallable() const = 0;
	virtual const char* GetSignature_RetType() const = 0;

public:

	const MAssThread::Id m_thread_id;
};


class ResultJob: public MAssCallThreadTransport::Job
{
public:

	ResultJob(const std::weak_ptr<MAssIpcCallTransport>& transport);

	void Invoke() override;

	std::vector<uint8_t> m_result;

private:

	std::weak_ptr<MAssIpcCallTransport> m_transport;
};


class CallJob: public MAssCallThreadTransport::Job
{
public:

	CallJob(const std::weak_ptr<MAssIpcCallTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
			std::unique_ptr<std::vector<uint8_t> > call_info_data);
	void Invoke() override;

	MAssIpcCallDataStream			m_call_info_data_str;
	std::shared_ptr<const CallInfo>	m_call_info;
	bool							m_send_return=false;

private:

	static MAssThread::Id MakeResultThreadId(const std::weak_ptr<MAssCallThreadTransport>& inter_thread);

private:

	std::unique_ptr<const std::vector<uint8_t> >		m_call_info_data;
	MAssThread::Id			m_result_thread_id;
	std::weak_ptr<MAssIpcCallTransport>					m_transport;
	std::weak_ptr<MAssCallThreadTransport>				m_inter_thread;
};


class ProcMap
{
public:

	std::shared_ptr<const CallInfo> FindCallInfo(const std::string& name, std::string& signature) const;
	MAssIpcCall_EnumerateData EnumerateHandlers();

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
		int unpack[]{0,((*params_type += MAssIpcType<TArgs>::NameValue(), *params_type += separator
#if !(_MSC_VER==1916)// Visual Studio 2017 compiler crashes trying compile this
						 , CheckSeparatorInName<MAssIpcType<TArgs>::name_value>() 
#endif

						 ),0)...};
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


