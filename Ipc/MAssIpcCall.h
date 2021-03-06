#pragma once

#include "MAssIpcCallDataStream.h"
#include <functional>
#include <memory>
#include "MAssCallThreadTransport.h"
#include "MAssIpcCallInternal.h"
#include "MAssIpcThreadSafe.h"
#include <list>
#include <map>

class MAssIpcCall
{
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

	void SetTransport(const std::weak_ptr<MAssIpcCallTransport>& transport);
	void SetTransport(const std::weak_ptr<MAssIpcPacketTransport>& transport);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRet(const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvoke(const std::string& proc_name, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	TRet WaitInvokeRetAlertable(const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvokeAlertable(const std::string& proc_name, const TArgs&... args) const;

	template<class TRet, class... TArgs>
	TRet WaitInvokeRet(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvoke(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	TRet WaitInvokeRetAlertable(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvokeAlertable(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const;


	template<class... TArgs>
	void AsyncInvoke(const std::string& proc_name, const TArgs&... args) const;

	template<class... TArgs>
	void AsyncInvoke(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const;


	template<class TRet, class... TArgs>
	static MAssIpcData::TPacketSize CalcCallSize(bool send_return, const std::string& proc_name, const TArgs&... args);

	template<class TDelegateW>
	void AddHandler(const std::string& proc_name, const TDelegateW& del,
					MAssThread::Id thread_id = MAssThread::c_no_thread);
	template<class TDelegateW>
	void AddHandler(const std::string& proc_name, const TDelegateW& del, const std::string& comment,
					MAssThread::Id thread_id = MAssThread::c_no_thread);

	void SetErrorHandler(TErrorHandler OnInvalidRemoteCall);

	void ProcessTransport();

	MAssIpcCall_EnumerateData EnumerateRemote() const;
	MAssIpcCall_EnumerateData EnumerateLocal() const;

	static constexpr char separator = MAssIpcCallInternal::separator;

private:

	static void SerializeCallSignature(MAssIpcCallDataStream& call_info, const std::string& proc_name,
									   bool send_return, const std::string& return_type,
									   const std::string& params_type);

	template<class TRet, class... TArgs>
	static void SerializeCall(MAssIpcCallDataStream& call_info, const std::string& proc_name, bool send_return, const TArgs&... args);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRetUnified(std::unique_ptr<MAssIpcData>& inplace_send_buffer, const std::string& proc_name, bool process_incoming_calls, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	void InvokeUnified(std::unique_ptr<MAssIpcData>& inplace_send_buffer,
					   const std::string& proc_name, MAssIpcCallDataStream* result_buf_wait_return,
					   bool process_incoming_calls,
					   const TArgs&... args) const;
	void InvokeRemote(MAssIpcCallDataStream& call_info_data, MAssIpcCallDataStream* result_buf_wait_return,
					  MAssIpcCallInternal::MAssIpcPacketParser::TCallId wait_response_id,
					  bool process_incoming_calls) const;

	static void DeserializeNameSignature(MAssIpcCallDataStream& call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type);

	MAssIpcCallInternal::MAssIpcPacketParser::TCallId NewCallId() const;
	
	MAssIpcCallDataStream ProcessTransportResponse(MAssIpcCallInternal::MAssIpcPacketParser::TCallId wait_response_id,
												   bool process_incoming_calls) const;


	struct CallDataBuffer
	{
		MAssIpcCallDataStream data;
		MAssIpcCallInternal::MAssIpcPacketParser::Header header;
	};

	CallDataBuffer ReceiveCallDataBuffer(std::unique_ptr<MAssIpcData>& packet) const;

private:


	struct PendingResponces
	{
		// all membersaccess only during unique_lock(m_lock)
		MAssIpcThreadSafe::mutex m_lock;

		MAssIpcCallInternal::MAssIpcPacketParser::TCallId m_incremental_id = 0;
		volatile bool m_another_thread_waiting_transport = false;
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

		struct CreateCallJobRes
		{
			std::shared_ptr<MAssIpcCallInternal::CallJob> obj;
			bool send_return = false;
			ErrorType error = ErrorType::unknown_error;
			std::string message;
		};

		CreateCallJobRes CreateCallJob(const std::shared_ptr<MAssIpcPacketTransport>& transport, 
									   MAssIpcCallDataStream& call_info_data, 
									   MAssIpcCallInternal::MAssIpcPacketParser::TCallId id) const;
		static void StoreReturnFailCall(MAssIpcCallDataStream* result_str, const CreateCallJobRes& call_job, 
										MAssIpcCallInternal::MAssIpcPacketParser::TCallId id);

	};

	friend class MAssIpcCall::Internals;

	std::shared_ptr<Internals>		m_int;
};


//-------------------------------------------------------

template<class TDelegateW>
void MAssIpcCall::AddHandler(const std::string& proc_name, const TDelegateW& del, MAssThread::Id thread_id)
{
	AddHandler(proc_name, del, std::string(), thread_id);
}

template<class TDelegateW>
void MAssIpcCall::AddHandler(const std::string& proc_name, const TDelegateW& del, const std::string& comment,
						 MAssThread::Id thread_id)
{
	const std::shared_ptr<MAssIpcCallInternal::CallInfo> call_info(new typename MAssIpcCallInternal::Impl_Selector<TDelegateW>::Res(del, thread_id));
	std::string params_type;
	MAssIpcCallInternal::ProcSignature<typename MAssIpcCallInternal::Impl_Selector<TDelegateW>::TDelProcUnified>::GetParams(&params_type);
	m_int->m_proc_map.AddProcSignature(proc_name, params_type, call_info, comment);
}

template<class TRet, class... TArgs>
MAssIpcData::TPacketSize MAssIpcCall::CalcCallSize(bool send_return, const std::string& proc_name, const TArgs&... args)
{
	MAssIpcCallDataStream measure_size;
	SerializeCall<TRet>(measure_size, proc_name, send_return, args...);
	return measure_size.GetWritePos()+MAssIpcCallInternal::MAssIpcPacketParser::c_net_call_packet_header_size;
}

template<class TRet, class... TArgs>
void MAssIpcCall::SerializeCall(MAssIpcCallDataStream& call_info, const std::string& proc_name, bool send_return, const TArgs&... args)
{
	typedef TRet(*TreatProc)(TArgs...);
	std::string return_type;
	std::string params_type;
	return_type = MAssIpcType<TRet>::NameValue();
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type);

	MAssIpcCall::SerializeCallSignature(call_info, proc_name, send_return, return_type, params_type);
	MAssIpcCallInternal::SerializeArgs(call_info, args...);
}

template<class TRet, class... TArgs>
void MAssIpcCall::InvokeUnified(std::unique_ptr<MAssIpcData>& inplace_send_buffer, 
								const std::string& proc_name, MAssIpcCallDataStream* result_buf_wait_return,
								bool process_incoming_calls,
								const TArgs&... args) const
{
	auto new_id = NewCallId();

	MAssIpcCallDataStream measure_size;
	SerializeCall<TRet>(measure_size, proc_name, (result_buf_wait_return!=nullptr), args...);

	MAssIpcCallDataStream call_info;
	if( inplace_send_buffer )
		call_info = CreateDataStreamInplace(inplace_send_buffer, measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, new_id);
	else
		call_info = CreateDataStream(m_int->m_transport, measure_size.GetWritePos(), MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, new_id);

	SerializeCall<TRet>(call_info, proc_name, (result_buf_wait_return!=nullptr), args...);

	InvokeRemote(call_info, result_buf_wait_return, new_id, process_incoming_calls);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetUnified(std::unique_ptr<MAssIpcData>& inplace_send_buffer, const std::string& proc_name, bool process_incoming_calls, const TArgs&... args) const
{
	static_assert(!std::is_same<TRet,void>::value, "can not be implicit void use function call explicitely return void");

	MAssIpcCallDataStream result;
	InvokeUnified<TRet>(inplace_send_buffer, proc_name, &result, process_incoming_calls, args...);
	{
		TRet res = {};
		if( result.IsDataBufferPresent() )
			result>>res;
		return res;
	}
}

//-------------------------------------------------------

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRet(const std::string& proc_name, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	return WaitInvokeRetUnified<TRet>(inplace, proc_name, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvoke(const std::string& proc_name, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(inplace, proc_name, &result_buf, false, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(const std::string& proc_name, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	InvokeUnified<void>(inplace, proc_name, nullptr, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvokeAlertable(const std::string& proc_name, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(inplace, proc_name, &result_buf, true, args...);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetAlertable(const std::string& proc_name, const TArgs&... args) const
{
	std::unique_ptr<MAssIpcData> inplace;
	return WaitInvokeRetUnified<TRet>(inplace, proc_name, true, args...);
}



template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRet(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const
{
	return WaitInvokeRetUnified<TRet>(inplace_send_buffer, proc_name, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvoke(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const
{
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(inplace_send_buffer, proc_name, &result_buf, false, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const
{
	InvokeUnified<void>(inplace_send_buffer, proc_name, nullptr, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvokeAlertable(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const
{
	MAssIpcCallDataStream result_buf;
	InvokeUnified<void>(inplace_send_buffer, proc_name, &result_buf, true, args...);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetAlertable(std::unique_ptr<MAssIpcData> inplace_send_buffer, const std::string& proc_name, const TArgs&... args) const
{
	return WaitInvokeRetUnified<TRet>(inplace_send_buffer, proc_name, true, args...);
}

//-------------------------------------------------------


MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
