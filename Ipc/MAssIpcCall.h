#pragma once

#include "MAssIpc_DataStream.h"
#include <memory>
#include "MAssIpc_Transthread.h"
#include "MAssIpc_Internals.h"
#include "MAssIpc_ThreadSafe.h"
#include "MAssIpc_RawString.h"
#include <list>
#include <map>

class MAssIpcCall
{
public:

	// During waiting response specify when to execute process incoming calls 
	enum struct ProcIn
	{
		later,
		now,
		bydefault,
	};

private:

	struct InvokeSetting
	{
		template<MAssIpc_Data::PacketSize N>
		constexpr InvokeSetting(const char(&str)[N])
			:proc_name{str,(N-1)}
		{
			static_assert(N>=1, "unexpected string size");
		}

		template<class T, typename std::enable_if<!std::is_array<T>::value, bool>::type = true>
		InvokeSetting(const T str)
			: proc_name{str}
		{
		}

		InvokeSetting(const std::string& str)
			: proc_name{str}
		{
		}

		InvokeSetting(const MAssIpcImpl::MAssIpc_RawString& proc_name, std::unique_ptr<MAssIpc_Data> inplace_send_buffer)
			:proc_name(proc_name)
			, inplace_send_buffer(std::move(inplace_send_buffer))
		{
		}

		InvokeSetting(const MAssIpcImpl::MAssIpc_RawString& proc_name, std::unique_ptr<MAssIpc_Data> inplace_send_buffer, ProcIn process_incoming_calls)
			:proc_name(proc_name)
			, inplace_send_buffer(std::move(inplace_send_buffer))
			, process_incoming_calls(process_incoming_calls)
		{
		}

		InvokeSetting(const MAssIpcImpl::MAssIpc_RawString& proc_name, ProcIn process_incoming_calls)
			:proc_name(proc_name)
			, process_incoming_calls(process_incoming_calls)
		{
		}

		MAssIpcImpl::MAssIpc_RawString proc_name;
		std::unique_ptr<MAssIpc_Data> inplace_send_buffer;
		ProcIn process_incoming_calls = ProcIn::bydefault;
	};

    template<class Sig>
	struct TInvokeSetting: public InvokeSetting
    {
        constexpr TInvokeSetting(InvokeSetting&& base):InvokeSetting(std::move(base)){}
    };


    template<class Sig> 
    struct IsSignature: public std::false_type{};

    template<class Ret, class... Args>
    struct IsSignature<Ret(Args...)>: public std::true_type{};

public:

	// constexpr MAssIpcCall::SigName<bool(float)> sig_Proc = {"Proc"};
    template<class FunctionSignature>
    struct SigName
    {
    public:

		using Signature = typename MAssIpcImpl::FuncSig<FunctionSignature>::FuncType;
        static_assert(IsSignature<Signature>::value, "Signature must be function type like Ret(Args...)");

		constexpr TInvokeSetting<SigName<Signature>> operator()(std::unique_ptr<MAssIpc_Data>&& inplace_send_buffer, ProcIn process_incoming_calls) const
		{
			return {InvokeSetting{proc_name, std::move(inplace_send_buffer), process_incoming_calls}};
		}

        constexpr TInvokeSetting<SigName<Signature>> operator()(std::unique_ptr<MAssIpc_Data>&& inplace_send_buffer) const
        {
            return {InvokeSetting{proc_name, std::move(inplace_send_buffer)}};
        }

		constexpr TInvokeSetting<SigName<Signature>> operator()(ProcIn process_incoming_calls) const
		{
			return {InvokeSetting{proc_name, process_incoming_calls}};
		}

		constexpr TInvokeSetting<SigName<Signature>> operator()() const
        {
            return {InvokeSetting{proc_name}};
        }

        const MAssIpcImpl::MAssIpc_RawString proc_name;
    };

public:

	friend class MAssIpcCall_AutoTest;

	using ErrorType = MAssIpcImpl::ErrorType;

	static constexpr const char* ErrorTypeToStr(ErrorType error)
	{
		return MAssIpcImpl::ErrorTypeToStr(error);
	};


	using CallInfo = MAssIpcImpl::CallInfo;
	using CallCountChanged = MAssIpcImpl::CallCountChanged;
	using ErrorOccured = MAssIpcImpl::ErrorOccured;


	MAssIpcCall(const MAssIpcCall&)=default;
	MAssIpcCall& operator=(const MAssIpcCall&)=default;
	MAssIpcCall(const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpc_TransportCopy>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);

	void SetTransport(const std::weak_ptr<MAssIpc_TransportCopy>& transport);
	void SetTransport(const std::weak_ptr<MAssIpc_TransportShare>& transport);
	void AddAllHandlers(const MAssIpcCall& other);
	void ClearAllHandlers();
	void ClearHandlersWithTag(const void* tag);

	void SetProcessIncomingCalls(bool process_incoming_calls_default);

	template<class Ret, class... Args>
	Ret WaitInvokeRet(InvokeSetting settings, const Args&... args) const;
	template<class... Args>
	void WaitInvoke(InvokeSetting settings, const Args&... args) const;

    template<class Ret_Check, class... Args_Check, class... Args_Local>
    Ret_Check WaitInvoke(TInvokeSetting<SigName<Ret_Check(Args_Check...)>> settings, Args_Local... args) const
    {
        return ConvertArgs<Ret_Check(Args_Check...)>::InvokeUnified(this, settings, args...);
    }

	template<class Ret_Check, class... Args_Check, class... Args_Local>
	Ret_Check WaitInvoke(SigName<Ret_Check(Args_Check...)> signame, Args_Local&&... args) const
    {
        return WaitInvoke(signame(), std::forward<Args_Local>(args)...);
    }


	template<class... Args>
	void AsyncInvoke(InvokeSetting settings, const Args&... args) const;

	template<class Ret_Check, class... Args_Check, class... Args_Local>
	void AsyncInvoke(TInvokeSetting<SigName<Ret_Check(Args_Check...)>> settings, Args_Local... args) const
	{
		ConvertArgs<Ret_Check(Args_Check...)>::AsyncInvokeUnified(this, settings, args...);
	}

	template<class Ret_Check, class... Args_Check, class... Args_Local>
	void AsyncInvoke(SigName<Ret_Check(Args_Check...)> signame, Args_Local&&... args) const
	{
		AsyncInvoke(signame(), std::forward<Args_Local>(args)...);
	}



	template<class Ret, class... Args>
	static MAssIpc_Data::PacketSize CalcCallSize(bool send_return, const MAssIpcImpl::MAssIpc_RawString& proc_name, const Args&... args);

	template<class Delegate>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del,
											   MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread());
	template<class Delegate>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const std::string& comment,
											   MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread());
	template<class Delegate>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const void* tag,
											   MAssIpc_TransthreadTarget::Id thread_id, const std::string& comment = {});
	template<class Delegate>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const void* tag);

	template<class Delegate>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const std::string& comment,
											   MAssIpc_TransthreadTarget::Id thread_id, const void* tag);// obsolete


	template<class Signature_Return, class Delegate, class... Signature_Args>
	std::shared_ptr<const CallInfo> AddHandler(const SigName<Signature_Return(Signature_Args...)>& signame, Delegate&& del, const void* tag,
											   MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread(), const std::string& comment = {})
	{
		static_assert(MAssIpcImpl::Check_is_signame_and_handler_describe_same_call_signatures(typename MAssIpcImpl::FuncSig<Signature_Return(Signature_Args...)>::FuncPtr{}, typename MAssIpcImpl::FuncSig<Delegate>::FuncPtr{}), "'signame' and 'del' must have same callable function signature");
		return AddHandler<Delegate>(signame.proc_name, std::forward<Delegate>(del), tag, thread_id, comment);
	}

	template<class Signature_Return, class... Signature_Args>
	std::shared_ptr<const CallInfo> AddCallInfo(const SigName<Signature_Return(Signature_Args...)>& signame)
	{
		return m_int.load()->m_proc_map.AddProcSignature(signame.proc_name, MAssIpcImpl::CallInfoImpl::MakeParamsType<Signature_Return(Signature_Args...)>(), {}, {});
	}

	std::shared_ptr<const ErrorOccured> SetHandler_ErrorOccured(const std::shared_ptr<const ErrorOccured>& new_val)
	{
		return m_int.load()->m_proc_map.SetErrorHandler(new_val);
	}

	template<class Delegate>
	std::shared_ptr<const ErrorOccured> SetHandler_ErrorOccured(Delegate&& del, const void* tag = nullptr, MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread())
	{
		static_assert(MAssIpcImpl::Check_is_signame_and_handler_describe_same_call_signatures(typename MAssIpcImpl::FuncSig<decltype(&ErrorOccured::Invoke)>::FuncPtr{}, typename MAssIpcImpl::FuncSig<Delegate>::FuncPtr{}), "Delegate signature not match");
		static_assert(MAssIpcImpl::IsHandlerTypeCompatible<Delegate>::value, "Delegate is not compatible del type");
		const std::shared_ptr<const ErrorOccured>& new_handler = std::make_shared<MAssIpcImpl::ErrorOccured_Imp<Delegate> >(std::forward<Delegate>(del), thread_id, tag);
		return SetHandler_ErrorOccured(new_handler);
	}


	std::shared_ptr<const CallCountChanged> SetHandler_CallCountChanged(const std::shared_ptr<const CallCountChanged>& new_val)
	{
		return m_int.load()->m_proc_map.SetCallCountChanged(new_val);
	}

	template<class Delegate>
	std::shared_ptr<const CallCountChanged> SetHandler_CallCountChanged(Delegate&& del, const void* tag=nullptr, MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread())
	{
		static_assert(MAssIpcImpl::Check_is_signame_and_handler_describe_same_call_signatures(typename MAssIpcImpl::FuncSig<decltype(&CallCountChanged::Invoke)>::FuncPtr{}, typename MAssIpcImpl::FuncSig<Delegate>::FuncPtr{}), "Delegate signature not match");
 		static_assert(MAssIpcImpl::IsHandlerTypeCompatible<Delegate>::value, "Delegate is not compatible del type");
		const std::shared_ptr<const CallCountChanged>& new_handler = std::make_shared<MAssIpcImpl::CallCountChanged_Imp<Delegate> >(std::forward<Delegate>(del), thread_id, tag);
		return SetHandler_CallCountChanged(new_handler);
	}

	void ProcessTransport();

	MAssIpcCall_EnumerateData EnumerateRemote() const;
	MAssIpcCall_EnumerateData EnumerateLocal() const;

	static constexpr decltype(MAssIpcImpl::separator) separator = MAssIpcImpl::separator;

private:

	template<class Ret, class... Args>
	static void SerializeCallSignature(MAssIpc_DataStream& call_info, const MAssIpcImpl::MAssIpc_RawString& proc_name, bool send_return);

	template<class Ret, class... Args>
	static void SerializeCall(MAssIpc_DataStream& call_info, const MAssIpcImpl::MAssIpc_RawString& proc_name, bool send_return, const Args&... args);

	template<class Ret, class... Args>
	Ret WaitInvokeRetUnified(InvokeSetting& settings, const Args&... args) const;
	template<class Ret, class... Args>
	void InvokeUnified(InvokeSetting& settings, MAssIpc_DataStream* result_buf_wait_return,
					   const Args&... args) const;
	void InvokeRemote(MAssIpc_DataStream& call_info_data, MAssIpc_DataStream* result_buf_wait_return,
					  MAssIpcImpl::MAssIpc_PacketParser::CallId wait_response_id,
					  bool process_incoming_calls) const;

    template<class Signature>
    struct ConvertArgs
    {
        static_assert(sizeof(Signature)==0, "Signature must be void or function type");
    };

    template<class Signature>
    friend struct ConvertArgs;

    template<class Ret_Check, class... Args_Check>
    struct ConvertArgs<Ret_Check(Args_Check...)>
    {
        template<class... Args_Local>
        static Ret_Check InvokeUnified(const MAssIpcCall* self, InvokeSetting& settings, Args_Local... args_local)
        {
            return self->WaitInvokeRetUnified<Ret_Check, Args_Check...>(settings, std::forward<Args_Local>(args_local)...);    
        }

		template<class... Args_Local>
		static Ret_Check AsyncInvokeUnified(const MAssIpcCall* self, InvokeSetting& settings, Args_Local... args_local)
		{
			static_assert(sizeof(Ret_Check)>=0, "Async invoke can not return value, must be void");
			return {};
		}
	};

    template<class... Args_Check>
    struct ConvertArgs<void(Args_Check...)>
    {
        template<class... Args_Local>
        static void InvokeUnified(const MAssIpcCall* self, InvokeSetting& settings, Args_Local... args_local)
        {
            MAssIpc_DataStream result_buf_wait_return;
            self->InvokeUnified<void,Args_Check...>(settings, &result_buf_wait_return, args_local...);
        }

		template<class... Args_Local>
		static void AsyncInvokeUnified(const MAssIpcCall* self, InvokeSetting& settings,
								  Args_Local... args_local)
		{
			self->InvokeUnified<void, Args_Check...>(settings, nullptr, args_local...);
		}
	};


	struct DeserializedFindCallInfo
	{
		MAssIpcImpl::MAssIpc_RawString name;
		bool send_return;
		MAssIpcImpl::MAssIpc_RawString return_type;
		MAssIpcImpl::MAssIpc_RawString params_type;
	};
	static DeserializedFindCallInfo DeserializeNameSignature(MAssIpc_DataStream& call_info);

	static inline bool IsDirectCall(const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable, const MAssIpc_TransthreadTarget::Id& thread_id);
	inline bool IsProcessIncomingCalls(ProcIn process_incoming_calls) const;
	MAssIpcImpl::MAssIpc_PacketParser::CallId NewCallId() const;
	
	MAssIpc_DataStream ProcessTransportResponse(MAssIpcImpl::MAssIpc_PacketParser::CallId wait_response_id,
												   bool process_incoming_calls) const;


	struct CallDataBuffer
	{
		MAssIpc_DataStream data;
		MAssIpcImpl::MAssIpc_PacketParser::Header header;
	};

	CallDataBuffer ReceiveCallDataBuffer(std::unique_ptr<const MAssIpc_Data>& packet) const;

private:

	struct LockCurrentThreadId
	{
		void lock();
		void unlock();

		bool IsCurrent() const;
		bool IsLocked() const;

	private:
		MAssIpc_ThreadSafe::id m_locked_id = MAssIpc_ThreadSafe::id();
	};

	struct PendingResponces
	{
		// all members access only during unique_lock(m_lock)
		MAssIpc_ThreadSafe::mutex m_lock;

		LockCurrentThreadId m_thread_waiting_transport;
		MAssIpcImpl::MAssIpc_PacketParser::CallId m_incremental_id = 0;
		MAssIpc_ThreadSafe::condition_variable m_write_cond;
		std::map<MAssIpcImpl::MAssIpc_PacketParser::CallId, CallDataBuffer> m_id_data_return;
		std::list<CallDataBuffer> m_data_incoming_call;
	};


	class Internals
	{
	public:

		MAssIpc_DataStream ProcessBuffer(CallDataBuffer buffer) const;

		MAssIpcImpl::ProcMap			m_proc_map;
		std::shared_ptr<MAssIpc_TransportShare>	m_transport_default;
		std::weak_ptr<MAssIpc_TransportShare>	m_transport;
		std::weak_ptr<MAssIpc_Transthread>		m_inter_thread_nullable;
		PendingResponces						m_pending_responses;
		bool									m_process_incoming_calls_default = true;

	private:

		void InvokeLocal(MAssIpc_DataStream& call_info_data, MAssIpcImpl::MAssIpc_PacketParser::CallId id) const;

		struct AnalizeInvokeDataRes
		{
			std::shared_ptr<MAssIpcImpl::CallInfoImpl> call_info;
			std::shared_ptr<MAssIpcImpl::InvokeRemoteBase> invoke_base;
			bool send_return = false;
			std::shared_ptr<const CallCountChanged>	on_call_count_changed;
			std::shared_ptr<MAssIpcImpl::ErrorJob::Arg> error_arg;
		};

		AnalizeInvokeDataRes AnalizeInvokeData(const std::shared_ptr<MAssIpc_TransportShare>& transport, const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable,
									   MAssIpc_DataStream& call_info_data, 
									   MAssIpcImpl::MAssIpc_PacketParser::CallId id) const;
		AnalizeInvokeDataRes ReportError_FindCallInfo(const DeserializedFindCallInfo& find_call_info, ErrorType error, 
													  const MAssIpcImpl::ProcMap::FindCallInfoRes& find_res, 
													  const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable) const;
		static void StoreReturnFailCall(MAssIpc_DataStream* result_str, const MAssIpcImpl::ErrorJob::Arg& error_arg,
										MAssIpcImpl::MAssIpc_PacketParser::CallId id);
		static void SendErrorMessage(const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable, 
									 const std::shared_ptr<const ErrorOccured>& on_invalid_remote_call, 
									 const std::shared_ptr<MAssIpcImpl::ErrorJob::Arg>& error_arg);
	};

	friend class MAssIpcCall::Internals;

	MAssIpcImpl::AtomicSharedPtr<Internals>		m_int;


public:
	class SetCallCountChangedGuard
	{
	public:
		SetCallCountChangedGuard(MAssIpcCall& ipc, const std::shared_ptr<const CallCountChanged>& handler)
			:m_ipc_int(ipc.m_int.load())
			, m_old_handler(m_ipc_int->m_proc_map.SetCallCountChanged(handler))
		{
		}

		~SetCallCountChangedGuard()
		{
			m_ipc_int->m_proc_map.SetCallCountChanged(m_old_handler);
		}

		template<class Delegate>
		static std::shared_ptr<const CallCountChanged> MakeHandler_CallCountChanged(Delegate&& del, const void* tag = nullptr, MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread())
		{
			static_assert(MAssIpcImpl::Check_is_signame_and_handler_describe_same_call_signatures(typename MAssIpcImpl::FuncSig<decltype(&CallCountChanged::Invoke)>::FuncPtr{}, typename MAssIpcImpl::FuncSig<Delegate>::FuncPtr{}), "Delegate signature not match");
			static_assert(MAssIpcImpl::IsHandlerTypeCompatible<Delegate>::value, "Delegate is not compatible del type");
			std::shared_ptr<const CallCountChanged> new_handler = std::make_shared<MAssIpcImpl::CallCountChanged_Imp<Delegate> >(std::forward<Delegate>(del), thread_id, tag);
			return new_handler;
		}


	private:

		std::shared_ptr<Internals>					m_ipc_int;
		std::shared_ptr<const CallCountChanged>	m_old_handler;
	};
	

	class HandlerGuard
	{
	public:
		HandlerGuard(MAssIpcCall& ipc, const void* clear_tag)
			:m_ipc_int(ipc.m_int.load())
			, m_clear_tag(clear_tag)
		{
		}

		~HandlerGuard()
		{
			m_ipc_int->m_proc_map.ClearProcsWithTag(m_clear_tag);
		}

	private:

		std::shared_ptr<Internals>		m_ipc_int;
		const void* const m_clear_tag;
	};

};

//-------------------------------------------------------
inline bool MAssIpcCall::IsDirectCall(const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable, const MAssIpc_TransthreadTarget::Id& thread_id)
{
	return ( !bool(inter_thread_nullable) || (thread_id==MAssIpc_TransthreadTarget::DirectCallPseudoId()) );
}

inline bool MAssIpcCall::IsProcessIncomingCalls(ProcIn process_incoming_calls) const
{
	switch( process_incoming_calls )
	{
		case ProcIn::later: return false;
		case ProcIn::now: return true;
		default:
		case ProcIn::bydefault: return m_int.load()->m_process_incoming_calls_default;
	}
}

//-------------------------------------------------------

template<class Delegate>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, MAssIpc_TransthreadTarget::Id thread_id)
{
	return AddHandler(proc_name, std::forward<Delegate>(del), nullptr, thread_id, std::string());
}

template<class Delegate>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const std::string& comment,
							 MAssIpc_TransthreadTarget::Id thread_id)
{
	return AddHandler(proc_name, std::forward<Delegate>(del), nullptr, thread_id, comment);
}

template<class Delegate>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const void* tag)
{
	return AddHandler(proc_name, std::forward<Delegate>(del), tag, MAssIpc_TransthreadTarget::CurrentThread(), std::string());
}

template<class Delegate>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const std::string& comment,
										   MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
{
	return AddHandler(proc_name, std::forward<Delegate>(del), tag, thread_id, comment);
}

template<class Delegate>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcImpl::MAssIpc_RawString& proc_name, Delegate&& del, const void* tag,
						 MAssIpc_TransthreadTarget::Id thread_id, const std::string& comment)
{
	static_assert(!MAssIpcImpl::Check_is_bind_expression<Delegate>::value, "can not deduce signature from bind_expression, use std::function<>(std::bind())");
	static_assert(MAssIpcImpl::IsHandlerTypeCompatible<Delegate>::value, "Delegate is not compatible del type");
	std::unique_ptr<MAssIpcImpl::InvokeRemoteBase> invoke(new typename MAssIpcImpl::Impl_Selector<Delegate>::Res(std::forward<Delegate>(del), thread_id, tag));
	return m_int.load()->m_proc_map.AddProcSignature(proc_name, MAssIpcImpl::CallInfoImpl::MakeParamsType<Delegate>(), std::move(invoke), comment);
}

template<class Ret, class... Args>
MAssIpc_Data::PacketSize MAssIpcCall::CalcCallSize(bool send_return, const MAssIpcImpl::MAssIpc_RawString& proc_name, const Args&... args)
{
	MAssIpc_DataStream measure_size;
	SerializeCall<Ret>(measure_size, proc_name, send_return, args...);
	return measure_size.GetWritePos()+MAssIpcImpl::MAssIpc_PacketParser::c_net_call_packet_header_size;
}

template<class Ret, class... Args>
void MAssIpcCall::SerializeCallSignature(MAssIpc_DataStream& call_info, const MAssIpcImpl::MAssIpc_RawString& proc_name, bool send_return)
{
	// call_info<<proc_name<<send_return<<return_type<<params_type;

	typedef Ret(*TreatProc)(Args...);
	constexpr MAssIpcImpl::MAssIpc_RawString return_type(MAssIpcType<Ret>::NameValue(), MAssIpcType<Ret>::NameLength());

	proc_name.Write(call_info);
	call_info<<send_return;
	return_type.Write(call_info);

	{
		MAssIpcImpl::ParamsTypeHolder_PacketSize params_type_calc;
		MAssIpcImpl::ProcSignature<TreatProc>::GetParams(&params_type_calc);

		call_info<<params_type_calc.m_size;
		MAssIpcImpl::ParamsTypeHolder_MAssIpcCallDataStream params_type_stream_write = {call_info};
		MAssIpcImpl::ProcSignature<TreatProc>::GetParams(&params_type_stream_write);
	}
}

template<class Ret, class... Args>
void MAssIpcCall::SerializeCall(MAssIpc_DataStream& call_info, const MAssIpcImpl::MAssIpc_RawString& proc_name, bool send_return, const Args&... args)
{
	MAssIpcCall::SerializeCallSignature<Ret, Args...>(call_info, proc_name, send_return);
	MAssIpcImpl::SerializeArgs(call_info, args...);
}

template<class Ret, class... Args>
void MAssIpcCall::InvokeUnified(InvokeSetting& settings, MAssIpc_DataStream* result_buf_wait_return,
								const Args&... args) const
{
	auto new_id = NewCallId();

	MAssIpc_DataStream measure_size;
	SerializeCall<Ret>(measure_size, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	MAssIpc_DataStream call_info;
	if( settings.inplace_send_buffer )
		call_info = CreateDataStreamInplace(std::move(settings.inplace_send_buffer), measure_size.GetWritePos(), MAssIpcImpl::MAssIpc_PacketParser::PacketType::pt_call, new_id);
	else
		call_info = CreateDataStream(m_int.load()->m_transport, measure_size.GetWritePos(), MAssIpcImpl::MAssIpc_PacketParser::PacketType::pt_call, new_id);

	SerializeCall<Ret>(call_info, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	const bool process_incoming_calls = IsProcessIncomingCalls(settings.process_incoming_calls);
	InvokeRemote(call_info, result_buf_wait_return, new_id, process_incoming_calls);
}

template<class Ret, class... Args>
Ret MAssIpcCall::WaitInvokeRetUnified(InvokeSetting& settings, const Args&... args) const
{
	static_assert(!std::is_same<Ret,void>::value, "can not be implicit void use function call explicitely return void");

	MAssIpc_DataStream result;
	InvokeUnified<Ret>(settings, &result, args...);
	{
		if( result.IsReadBufferPresent() )
			return MAssIpcImpl::ReadFromStream<Ret>(result);
		return MAssIpcImpl::MakeDefault<Ret>();
	}
}

//-------------------------------------------------------

template<class Ret, class... Args>
Ret MAssIpcCall::WaitInvokeRet(InvokeSetting settings, const Args&... args) const
{
	return WaitInvokeRetUnified<Ret>(settings, args...);
}

template<class... Args>
void MAssIpcCall::WaitInvoke(InvokeSetting settings, const Args&... args) const
{
	MAssIpc_DataStream result_buf;
	InvokeUnified<void>(settings, &result_buf, args...);
}

template<class... Args>
void MAssIpcCall::AsyncInvoke(InvokeSetting settings, const Args&... args) const
{
	InvokeUnified<void>(settings, nullptr, args...);
}

//-------------------------------------------------------


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
