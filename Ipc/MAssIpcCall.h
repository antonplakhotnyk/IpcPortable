#pragma once

#include "MAssIpc_DataStream.h"
#include <functional>
#include <memory>
#include "MAssIpc_Transthread.h"
#include "MAssIpc_Internals.h"
#include "MAssIpc_ThreadSafe.h"
#include "MAssIpc_RawString.h"
#include <list>
#include <map>

class MAssIpcCall
{
private:

	struct InvokeSetting
	{
		template<MAssIpc_Data::TPacketSize N>
		constexpr InvokeSetting(const char(&str)[N])
			:proc_name{str,(N-1)}
		{
			static_assert(N>=1, "unexpected string size");
		}

		template<class T, typename = typename std::enable_if<!std::is_array<T>::value>::type>
		InvokeSetting(const T str)
			: proc_name{str}
		{
		}

		InvokeSetting(const std::string& str)
			: proc_name{str}
		{
		}

		InvokeSetting(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, std::unique_ptr<MAssIpc_Data> inplace_send_buffer)
			:proc_name(proc_name)
			, inplace_send_buffer(std::move(inplace_send_buffer))
		{
		}

		MAssIpcCallInternal::MAssIpc_RawString proc_name;
		std::unique_ptr<MAssIpc_Data> inplace_send_buffer;
	};


public:

	friend class MAssIpcCall_AutoTest;

	enum struct ErrorType: uint8_t
	{
		unknown_error,
		no_matching_call_name_parameters,
		no_matching_call_return_type,
		respond_no_matching_call_name_parameters,
		respond_no_matching_call_return_type,
	};

	static constexpr const char* ErrorTypeToStr(ErrorType error)
	{
		switch( error )
		{
			default:
			case ErrorType::unknown_error: return "unknown_error";
			case ErrorType::no_matching_call_name_parameters: return "no_matching_call_name_parameters";
			case ErrorType::no_matching_call_return_type: return "no_matching_call_return_type";
			case ErrorType::respond_no_matching_call_name_parameters: return "respond_no_matching_call_name_parameters";
			case ErrorType::respond_no_matching_call_return_type: return "respond_no_matching_call_return_type";
		};
	};


	using TErrorHandler = std::function<void(ErrorType et, std::string message)>;
	
	using CallInfo = MAssIpcCallInternal::CallInfo;
	using TCallCountChanged = MAssIpcCallInternal::TCallCountChanged;


	MAssIpcCall(const MAssIpcCall&)=default;
	MAssIpcCall& operator=(const MAssIpcCall&) = default;
	MAssIpcCall(const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpc_TransportCopy>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable);

	void SetTransport(const std::weak_ptr<MAssIpc_TransportCopy>& transport);
	void SetTransport(const std::weak_ptr<MAssIpc_TransportShare>& transport);
	void AddAllHandlers(const MAssIpcCall& other);
	void ClearAllHandlers();
	void ClearHandlersWithTag(const void* tag);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRet(InvokeSetting settings, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvoke(InvokeSetting settings, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	TRet WaitInvokeRetAlertable(InvokeSetting settings, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvokeAlertable(InvokeSetting settings, const TArgs&... args) const;

	template<class... TArgs>
	void AsyncInvoke(InvokeSetting settings, const TArgs&... args) const;


	template<class TRet, class... TArgs>
	static MAssIpc_Data::TPacketSize CalcCallSize(bool send_return, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TArgs&... args);

	template<class TDelegateW>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del,
					MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_Transthread::NoThread());
	template<class TDelegateW>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del, const std::string& comment,
					MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_Transthread::NoThread());
	template<class TDelegateW>
	std::shared_ptr<const CallInfo> AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del, const std::string& comment,
					MAssIpc_TransthreadTarget::Id thread_id, const void* tag);

	void SetErrorHandler(TErrorHandler OnInvalidRemoteCall);
	TCallCountChanged SetCallCountChanged(const TCallCountChanged& OnCallCountChanged);

	void ProcessTransport();

	MAssIpcCall_EnumerateData EnumerateRemote() const;
	MAssIpcCall_EnumerateData EnumerateLocal() const;

	static constexpr decltype(MAssIpcCallInternal::separator) separator = MAssIpcCallInternal::separator;

private:

	template<class TRet, class... TArgs>
	static void SerializeCallSignature(MAssIpc_DataStream& call_info, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, bool send_return);

	template<class TRet, class... TArgs>
	static void SerializeCall(MAssIpc_DataStream& call_info, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, bool send_return, const TArgs&... args);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRetUnified(InvokeSetting& settings, bool process_incoming_calls, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	void InvokeUnified(InvokeSetting& settings, MAssIpc_DataStream* result_buf_wait_return,
					   bool process_incoming_calls,
					   const TArgs&... args) const;
	void InvokeRemote(MAssIpc_DataStream& call_info_data, MAssIpc_DataStream* result_buf_wait_return,
					  MAssIpcCallInternal::MAssIpc_PacketParser::TCallId wait_response_id,
					  bool process_incoming_calls) const;

	struct DeserializedFindCallInfo
	{
		MAssIpcCallInternal::MAssIpc_RawString name;
		bool send_return;
		MAssIpcCallInternal::MAssIpc_RawString return_type;
		MAssIpcCallInternal::MAssIpc_RawString params_type;
	};
	static DeserializedFindCallInfo DeserializeNameSignature(MAssIpc_DataStream& call_info);

	MAssIpcCallInternal::MAssIpc_PacketParser::TCallId NewCallId() const;
	
	MAssIpc_DataStream ProcessTransportResponse(MAssIpcCallInternal::MAssIpc_PacketParser::TCallId wait_response_id,
												   bool process_incoming_calls) const;


	struct CallDataBuffer
	{
		MAssIpc_DataStream data;
		MAssIpcCallInternal::MAssIpc_PacketParser::Header header;
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
		MAssIpcCallInternal::MAssIpc_PacketParser::TCallId m_incremental_id = 0;
		MAssIpc_ThreadSafe::condition_variable m_write_cond;
		std::map<MAssIpcCallInternal::MAssIpc_PacketParser::TCallId, CallDataBuffer> m_id_data_return;
		std::list<CallDataBuffer> m_data_incoming_call;
	};


	class Internals
	{
	public:

		MAssIpc_DataStream ProcessBuffer(CallDataBuffer buffer) const;

		MAssIpcCallInternal::ProcMap			m_proc_map;
		std::shared_ptr<MAssIpc_TransportShare>	m_transport_default;
		std::weak_ptr<MAssIpc_TransportShare>	m_transport;
		std::weak_ptr<MAssIpc_Transthread>	m_inter_thread_nullable;
		TErrorHandler							m_OnInvalidRemoteCall;
		std::shared_ptr<TCallCountChanged>		m_OnCallCountChanged = std::make_shared<TCallCountChanged>();
		
		PendingResponces						m_pending_responses;

		TCallCountChanged SetCallCountChanged(const TCallCountChanged& OnCallCountChanged)
		{
			TCallCountChanged& new_handler = *m_OnCallCountChanged.get();
			TCallCountChanged old_handler = new_handler;
			new_handler = OnCallCountChanged;
			return old_handler;
		}

	private:

		void InvokeLocal(MAssIpc_DataStream& call_info_data, MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const;

		struct AnalizeInvokeDataRes
		{
			std::shared_ptr<MAssIpcCallInternal::CallInfoImpl> call_info;
			bool send_return = false;
			ErrorType error = ErrorType::unknown_error;
			std::string message;
		};

		AnalizeInvokeDataRes AnalizeInvokeData(const std::shared_ptr<MAssIpc_TransportShare>& transport, 
									   MAssIpc_DataStream& call_info_data, 
									   MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const;
		AnalizeInvokeDataRes ReportError_FindCallInfo(const DeserializedFindCallInfo& find_call_info, ErrorType error) const;
		static void StoreReturnFailCall(MAssIpc_DataStream* result_str, const AnalizeInvokeDataRes& call_job,
										MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id);
	};

	friend class MAssIpcCall::Internals;

	std::shared_ptr<Internals>		m_int;


public:
	class SetCallCountChangedGuard
	{
	public:
		SetCallCountChangedGuard(MAssIpcCall& ipc, const TCallCountChanged& handler)
			:m_ipc_int(ipc.m_int)
			, m_old_handler(ipc.m_int->SetCallCountChanged(handler))
		{
		}

		~SetCallCountChangedGuard()
		{
			m_ipc_int->SetCallCountChanged(m_old_handler);
		}

	private:

		std::shared_ptr<Internals>		m_ipc_int;
		TCallCountChanged		m_old_handler;
	};
	

	class HandlerGuard
	{
	public:
		HandlerGuard(MAssIpcCall& ipc, const void* clear_tag)
			:m_ipc_int(ipc.m_int)
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

template<class TDelegateW>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del, MAssIpc_TransthreadTarget::Id thread_id)
{
	return AddHandler(proc_name, del, std::string(), thread_id, nullptr);
}

template<class TDelegateW>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del, const std::string& comment,
							 MAssIpc_TransthreadTarget::Id thread_id)
{
	return AddHandler(proc_name, del, comment, thread_id, nullptr);
}

template<class TDelegateW>
std::shared_ptr<const MAssIpcCall::CallInfo> MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TDelegateW& del, const std::string& comment,
						 MAssIpc_TransthreadTarget::Id thread_id, const void* tag)
{
	static_assert(!std::is_bind_expression<TDelegateW>::value, "can not deduce signature from bind_expression, use std::function<>(std::bind())");
	const std::shared_ptr<MAssIpcCallInternal::CallInfoImpl> call_info(std::make_shared<typename MAssIpcCallInternal::Impl_Selector<TDelegateW>::Res>(del, thread_id, proc_name));
	return m_int->m_proc_map.AddProcSignature(call_info, comment, tag);
}

template<class TRet, class... TArgs>
MAssIpc_Data::TPacketSize MAssIpcCall::CalcCallSize(bool send_return, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, const TArgs&... args)
{
	MAssIpc_DataStream measure_size;
	SerializeCall<TRet>(measure_size, proc_name, send_return, args...);
	return measure_size.GetWritePos()+MAssIpcCallInternal::MAssIpc_PacketParser::c_net_call_packet_header_size;
}

template<class TRet, class... TArgs>
void MAssIpcCall::SerializeCallSignature(MAssIpc_DataStream& call_info, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, bool send_return)
{
	// call_info<<proc_name<<send_return<<return_type<<params_type;

	typedef TRet(*TreatProc)(TArgs...);
	constexpr MAssIpcCallInternal::MAssIpc_RawString return_type(MAssIpcType<TRet>::NameValue(), MAssIpcType<TRet>::NameLength());

	proc_name.Write(call_info);
	call_info<<send_return;
	return_type.Write(call_info);

	{
		MAssIpcCallInternal::ParamsTypeHolder_TPacketSize params_type_calc;
		MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_calc);

		call_info<<params_type_calc.m_size;
		MAssIpcCallInternal::ParamsTypeHolder_MAssIpcCallDataStream params_type_stream_write = {call_info};
		MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type_stream_write);
	}
}

template<class TRet, class... TArgs>
void MAssIpcCall::SerializeCall(MAssIpc_DataStream& call_info, const MAssIpcCallInternal::MAssIpc_RawString& proc_name, bool send_return, const TArgs&... args)
{
	MAssIpcCall::SerializeCallSignature<TRet, TArgs...>(call_info, proc_name, send_return);
	MAssIpcCallInternal::SerializeArgs(call_info, args...);
}

template<class TRet, class... TArgs>
void MAssIpcCall::InvokeUnified(InvokeSetting& settings, MAssIpc_DataStream* result_buf_wait_return,
								bool process_incoming_calls,
								const TArgs&... args) const
{
	auto new_id = NewCallId();

	MAssIpc_DataStream measure_size;
	SerializeCall<TRet>(measure_size, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	MAssIpc_DataStream call_info;
	if( settings.inplace_send_buffer )
		call_info = CreateDataStreamInplace(std::move(settings.inplace_send_buffer), measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpc_PacketParser::PacketType::pt_call, new_id);
	else
		call_info = CreateDataStream(m_int->m_transport, measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpc_PacketParser::PacketType::pt_call, new_id);

	SerializeCall<TRet>(call_info, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	InvokeRemote(call_info, result_buf_wait_return, new_id, process_incoming_calls);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetUnified(InvokeSetting& settings, bool process_incoming_calls, const TArgs&... args) const
{
	static_assert(!std::is_same<TRet,void>::value, "can not be implicit void use function call explicitely return void");

	MAssIpc_DataStream result;
	InvokeUnified<TRet>(settings, &result, process_incoming_calls, args...);
	{
		TRet res = {};
		if( result.IsReadBufferPresent() )
			result>>res;
		return res;
	}
}

//-------------------------------------------------------

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRet(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpc_Data> inplace;
	return WaitInvokeRetUnified<TRet>(settings, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvoke(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpc_Data> inplace;
	MAssIpc_DataStream result_buf;
	InvokeUnified<void>(settings, &result_buf, false, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpc_Data> inplace;
	InvokeUnified<void>(settings, nullptr, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvokeAlertable(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpc_Data> inplace;
	MAssIpc_DataStream result_buf;
	InvokeUnified<void>(settings, &result_buf, true, args...);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetAlertable(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpc_Data> inplace;
	return WaitInvokeRetUnified<TRet>(settings, true, args...);
}

//-------------------------------------------------------


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
