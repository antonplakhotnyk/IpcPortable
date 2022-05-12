#pragma once

#include "MAssIpcCallDataStream.h"
#include <functional>
#include <memory>
#include "MAssCallThreadTransport.h"
#include "MAssIpcCallInternal.h"
#include "MAssIpcThreadSafe.h"
#include "MAssIpcRawString.h"
#include <list>
#include <map>

class MAssIpcCall
{
private:

	struct InvokeSetting
	{
		template<MAssIpcData::TPacketSize N>
		constexpr InvokeSetting(const char(&str)[N])
			:proc_name{str,(N-1)}
		{
			static_assert(N>=1, "unexpected string size");
		}

		template<class T, typename = typename std::enable_if<!std::is_array<T>::value>::type>
		InvokeSetting(const T str)
			: proc_name{str,strlen(str)}
		{
		}

		InvokeSetting(const std::string& str)
			: proc_name{str}
		{
		}

		InvokeSetting(const MAssIpcCallInternal::MAssIpcRawString& proc_name, std::unique_ptr<MAssIpcData> inplace_send_buffer)
			:proc_name(proc_name)
			, inplace_send_buffer(std::move(inplace_send_buffer))
		{
		}

		MAssIpcCallInternal::MAssIpcRawString proc_name;
		std::unique_ptr<MAssIpcData> inplace_send_buffer;
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

	typedef std::function<void(ErrorType et, std::string message)> TErrorHandler;


	MAssIpcCall(const MAssIpcCall&)=default;
	MAssIpcCall& operator=(const MAssIpcCall&) = default;
	MAssIpcCall(const std::weak_ptr<MAssCallThreadTransport>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpcCallTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread_nullable);
	MAssIpcCall(const std::weak_ptr<MAssIpcPacketTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread_nullable);

	void SetTransport(const std::weak_ptr<MAssIpcCallTransport>& transport);
	void SetTransport(const std::weak_ptr<MAssIpcPacketTransport>& transport);
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
	static MAssIpcData::TPacketSize CalcCallSize(bool send_return, const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TArgs&... args);

	template<class TDelegateW>
	void AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del,
					MAssIpcThreadTransportTarget::Id thread_id = MAssCallThreadTransport::NoThread());
	template<class TDelegateW>
	void AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del, const std::string& comment,
					MAssIpcThreadTransportTarget::Id thread_id = MAssCallThreadTransport::NoThread());
	template<class TDelegateW>
	void AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del, const std::string& comment,
					MAssIpcThreadTransportTarget::Id thread_id, const void* tag);

	void SetErrorHandler(TErrorHandler OnInvalidRemoteCall);

	void ProcessTransport();

	MAssIpcCall_EnumerateData EnumerateRemote() const;
	MAssIpcCall_EnumerateData EnumerateLocal() const;

	static constexpr decltype(MAssIpcCallInternal::separator) separator = MAssIpcCallInternal::separator;

private:

	template<class TRet, class... TArgs>
	static void SerializeCallSignature(MAssIpcCallDataStream& call_info, const MAssIpcCallInternal::MAssIpcRawString& proc_name, bool send_return);

	template<class TRet, class... TArgs>
	static void SerializeCall(MAssIpcCallDataStream& call_info, const MAssIpcCallInternal::MAssIpcRawString& proc_name, bool send_return, const TArgs&... args);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRetUnified(InvokeSetting& settings, bool process_incoming_calls, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	void InvokeUnified(InvokeSetting& settings, MAssIpcCallDataStream* result_buf_wait_return,
					   bool process_incoming_calls,
					   const TArgs&... args) const;
	void InvokeRemote(MAssIpcCallDataStream& call_info_data, MAssIpcCallDataStream* result_buf_wait_return,
					  MAssIpcCallInternal::MAssIpcPacketParser::TCallId wait_response_id,
					  bool process_incoming_calls) const;

	struct DeserializedFindCallInfo
	{
		MAssIpcCallInternal::MAssIpcRawString name;
		bool send_return;
		MAssIpcCallInternal::MAssIpcRawString return_type;
		MAssIpcCallInternal::MAssIpcRawString params_type;
	};
	static DeserializedFindCallInfo DeserializeNameSignature(MAssIpcCallDataStream& call_info);

	MAssIpcCallInternal::MAssIpcPacketParser::TCallId NewCallId() const;
	
	MAssIpcCallDataStream ProcessTransportResponse(MAssIpcCallInternal::MAssIpcPacketParser::TCallId wait_response_id,
												   bool process_incoming_calls) const;


	struct CallDataBuffer
	{
		MAssIpcCallDataStream data;
		MAssIpcCallInternal::MAssIpcPacketParser::Header header;
	};

	CallDataBuffer ReceiveCallDataBuffer(std::unique_ptr<const MAssIpcData>& packet) const;

private:

	struct LockCurrentThreadId
	{
		void lock();
		void unlock();

		bool IsCurrent() const;
		bool IsLocked() const;

	private:
		MAssIpcThreadSafe::id m_locked_id = MAssIpcThreadSafe::id();
	};

	struct PendingResponces
	{
		// all membersaccess only during unique_lock(m_lock)
		MAssIpcThreadSafe::mutex m_lock;

		LockCurrentThreadId m_thread_waiting_transport;
		MAssIpcCallInternal::MAssIpcPacketParser::TCallId m_incremental_id = 0;
		MAssIpcThreadSafe::condition_variable m_write_cond;
		std::map<MAssIpcCallInternal::MAssIpcPacketParser::TCallId, CallDataBuffer> m_id_data_return;
		std::list<CallDataBuffer> m_data_incoming_call;
	};


	class Internals
	{
	public:

		MAssIpcCallDataStream ProcessBuffer(CallDataBuffer buffer) const;

		MAssIpcCallInternal::ProcMap			m_proc_map;
		std::shared_ptr<MAssIpcPacketTransport>	m_transport_default;
		std::weak_ptr<MAssIpcPacketTransport>	m_transport;
		std::weak_ptr<MAssCallThreadTransport>	m_inter_thread_nullable;
		TErrorHandler							m_OnInvalidRemoteCall;
		
		PendingResponces						m_pending_responses;

	private:

		void InvokeLocal(MAssIpcCallDataStream& call_info_data, MAssIpcCallInternal::MAssIpcPacketParser::TCallId id) const;

		struct AnalizeInvokeDataRes
		{
			std::shared_ptr<const MAssIpcCallInternal::CallInfo> call_info;
			bool send_return = false;
			ErrorType error = ErrorType::unknown_error;
			std::string message;
		};

		AnalizeInvokeDataRes AnalizeInvokeData(const std::shared_ptr<MAssIpcPacketTransport>& transport, 
									   MAssIpcCallDataStream& call_info_data, 
									   MAssIpcCallInternal::MAssIpcPacketParser::TCallId id) const;
		AnalizeInvokeDataRes ReportError_FindCallInfo(const DeserializedFindCallInfo& find_call_info, ErrorType error) const;
		static void StoreReturnFailCall(MAssIpcCallDataStream* result_str, const AnalizeInvokeDataRes& call_job,
										MAssIpcCallInternal::MAssIpcPacketParser::TCallId id);
	};

	friend class MAssIpcCall::Internals;

	std::shared_ptr<Internals>		m_int;
};


//-------------------------------------------------------

template<class TDelegateW>
void MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del, MAssIpcThreadTransportTarget::Id thread_id)
{
	AddHandler(proc_name, del, std::string(), thread_id, nullptr);
}

template<class TDelegateW>
void MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del, const std::string& comment,
							 MAssIpcThreadTransportTarget::Id thread_id)
{
	AddHandler(proc_name, del, std::string(), thread_id, nullptr);
}

template<class TDelegateW>
void MAssIpcCall::AddHandler(const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TDelegateW& del, const std::string& comment,
						 MAssIpcThreadTransportTarget::Id thread_id, const void* tag)
{
	static_assert(!std::is_bind_expression<TDelegateW>::value, "can not deduce signature from bind_expression, use std::function<>(std::bind())");
	const std::shared_ptr<MAssIpcCallInternal::CallInfo> call_info(std::make_shared<typename MAssIpcCallInternal::Impl_Selector<TDelegateW>::Res>(del, thread_id, proc_name));

	m_int->m_proc_map.AddProcSignature(call_info, comment, tag);
}

template<class TRet, class... TArgs>
MAssIpcData::TPacketSize MAssIpcCall::CalcCallSize(bool send_return, const MAssIpcCallInternal::MAssIpcRawString& proc_name, const TArgs&... args)
{
	MAssIpcCallDataStream measure_size;
	SerializeCall<TRet>(measure_size, proc_name, send_return, args...);
	return measure_size.GetWritePos()+MAssIpcCallInternal::MAssIpcPacketParser::c_net_call_packet_header_size;
}

template<class TRet, class... TArgs>
void MAssIpcCall::SerializeCallSignature(MAssIpcCallDataStream& call_info, const MAssIpcCallInternal::MAssIpcRawString& proc_name, bool send_return)
{
	// call_info<<proc_name<<send_return<<return_type<<params_type;

	typedef TRet(*TreatProc)(TArgs...);
	constexpr MAssIpcCallInternal::MAssIpcRawString return_type(MAssIpcType<TRet>::NameValue(), MAssIpcType<TRet>::NameLength());

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
void MAssIpcCall::SerializeCall(MAssIpcCallDataStream& call_info, const MAssIpcCallInternal::MAssIpcRawString& proc_name, bool send_return, const TArgs&... args)
{
	MAssIpcCall::SerializeCallSignature<TRet, TArgs...>(call_info, proc_name, send_return);
	MAssIpcCallInternal::SerializeArgs(call_info, args...);
}

template<class TRet, class... TArgs>
void MAssIpcCall::InvokeUnified(InvokeSetting& settings, MAssIpcCallDataStream* result_buf_wait_return,
								bool process_incoming_calls,
								const TArgs&... args) const
{
	auto new_id = NewCallId();

	MAssIpcCallDataStream measure_size;
	SerializeCall<TRet>(measure_size, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	MAssIpcCallDataStream call_info;
	if( settings.inplace_send_buffer )
		call_info = CreateDataStreamInplace(std::move(settings.inplace_send_buffer), measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, new_id);
	else
		call_info = CreateDataStream(m_int->m_transport, measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, new_id);

	SerializeCall<TRet>(call_info, settings.proc_name, (result_buf_wait_return!=nullptr), args...);

	InvokeRemote(call_info, result_buf_wait_return, new_id, process_incoming_calls);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetUnified(InvokeSetting& settings, bool process_incoming_calls, const TArgs&... args) const
{
	static_assert(!std::is_same<TRet,void>::value, "can not be implicit void use function call explicitely return void");

	MAssIpcCallDataStream result;
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
	std::unique_ptr<MAssIpcData> inplace;
	return WaitInvokeRetUnified<TRet>(settings, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvoke(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(settings, &result_buf, false, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	InvokeUnified<void>(settings, nullptr, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvokeAlertable(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(settings, &result_buf, true, args...);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetAlertable(InvokeSetting settings, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	return WaitInvokeRetUnified<TRet>(settings, true, args...);
}

//-------------------------------------------------------


MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
