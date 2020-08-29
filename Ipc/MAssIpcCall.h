#pragma once

#include "MAssIpcCallDataStream.h"
#include <functional>
#include <memory>
#include "MAssIpcCallPacket.h"
#include "MAssCallThreadTransport.h"
#include "MAssIpcCallInternal.h"
#include <mutex>
#include <list>
#include <map>
#include <condition_variable>

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

	template<class TRet, class... TArgs>
	TRet WaitInvokeRet(const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvoke(const std::string& proc_name, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	TRet WaitInvokeRetAlertable(const std::string& proc_name, const TArgs&... args) const;
	template<class... TArgs>
	void WaitInvokeAlertable(const std::string& proc_name, const TArgs&... args) const;

	

	template<class... TArgs>
	void AsyncInvoke(const std::string& proc_name, const TArgs&... args) const;


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

	static void SerializeCallSignature(MAssIpcCallDataStream* call_info, const std::string& proc_name,
									   bool send_return, const std::string& return_type,
									   const std::string& params_type);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRetUnified(const std::string& proc_name, bool process_incoming_calls, const TArgs&... args) const;
	template<class TRet, class... TArgs>
	void InvokeUnified(const std::string& proc_name, std::vector<uint8_t>* result_buf_wait_return, 
					   bool process_incoming_calls,
					   const TArgs&... args) const;
	void InvokeRemote(const std::vector<uint8_t>& call_info_data, std::vector<uint8_t>* result, 
					  MAssIpcCallInternal::MAssIpcCallPacket::TCallId response_id,
					  bool process_incoming_calls) const;

	static void DeserializeNameSignature(MAssIpcCallDataStream* call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type);

	MAssIpcCallInternal::MAssIpcCallPacket::TCallId NewCallId() const;
	
	void ProcessTransportResponse(const std::shared_ptr<MAssIpcCallTransport>& transport,
								  std::vector<uint8_t>* result, 
								  MAssIpcCallInternal::MAssIpcCallPacket::TCallId response_id,
								  bool process_incoming_calls,
								  bool wait_incoming_data) const;


	struct CallDataBuffer
	{
		MAssIpcCallInternal::MAssIpcCallPacket::Header header;
		std::unique_ptr<std::vector<uint8_t> > data;
	};

	CallDataBuffer ReceiveCallDataBuffer(const std::shared_ptr<MAssIpcCallTransport>& transport) const;

private:


	struct PendingResponces
	{
		// all membersaccess only during unique_lock(m_lock)
		std::mutex m_lock;

		MAssIpcCallInternal::MAssIpcCallPacket::TCallId m_incremental_id = 0;
		volatile bool m_another_thread_waiting_transport = false;
		std::condition_variable m_write_cond;
		std::map<MAssIpcCallInternal::MAssIpcCallPacket::TCallId, CallDataBuffer> m_id_data_return;
		std::list<CallDataBuffer> m_data_incoming_call;
	};


	class Internals
	{
	public:

		void ProcessBuffer(CallDataBuffer buffer, std::vector<uint8_t>* result, const std::shared_ptr<MAssIpcCallTransport>& transport) const;

		MAssIpcCallInternal::ProcMap				m_proc_map;
		MAssIpcCallInternal::MAssIpcCallPacket		m_packet_parser;
		std::weak_ptr<MAssIpcCallTransport>			m_transport;
		std::weak_ptr<MAssCallThreadTransport>		m_inter_thread_nullable;
		TErrorHandler								m_OnInvalidRemoteCall;
		
		PendingResponces							m_pending_responses;

	private:

		void InvokeLocal(std::unique_ptr<std::vector<uint8_t> > call_info_data, MAssIpcCallInternal::MAssIpcCallPacket::TCallId id) const;

		struct CreateCallJobRes
		{
			std::shared_ptr<MAssIpcCallInternal::CallJob> obj;
			bool send_return = false;
			ErrorType error = ErrorType::unknown_error;
			std::string message;
		};

		CreateCallJobRes CreateCallJob(const std::shared_ptr<MAssIpcCallTransport>& transport, std::unique_ptr<std::vector<uint8_t> > call_info_data, MAssIpcCallInternal::MAssIpcCallPacket::TCallId id) const;

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
void MAssIpcCall::InvokeUnified(const std::string& proc_name, std::vector<uint8_t>* result_buf_wait_return, 
								bool process_incoming_calls,
								const TArgs&... args) const
{
	typedef TRet(*TreatProc)(TArgs...);
	std::string return_type;
	std::string params_type;
	return_type = MAssIpcType<TRet>::NameValue();
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type);

	std::vector<uint8_t> call_info_data_buf;
	MAssIpcCallDataStream call_info(&call_info_data_buf);

	auto new_id = NewCallId();
	MAssIpcCallInternal::MAssIpcCallPacket::PacketHeaderAllocate(&call_info_data_buf, MAssIpcCallInternal::MAssIpcCallPacket::PacketType::pt_call, new_id);
	MAssIpcCall::SerializeCallSignature(&call_info, proc_name, (result_buf_wait_return!=nullptr), return_type, params_type);
	MAssIpcCallInternal::SerializeArgs(&call_info, args...);
	MAssIpcCallInternal::MAssIpcCallPacket::PacketHeaderUpdateSize(&call_info_data_buf);

	InvokeRemote(call_info_data_buf, result_buf_wait_return, new_id, process_incoming_calls);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetUnified(const std::string& proc_name, bool process_incoming_calls, const TArgs&... args) const
{
	static_assert(!std::is_same<TRet,void>::value, "can not be implicit void use function call explicitely return void");

	std::vector<uint8_t> result_buf;
	InvokeUnified<TRet>(proc_name, &result_buf, process_incoming_calls, args...);
	{
		TRet res = {};
		if( !result_buf.empty() )
		{
			MAssIpcCallDataStream result(&result_buf[0], result_buf.size());
			result>>res;
		}
		return res;
	}
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRet(const std::string& proc_name, const TArgs&... args) const
{
	return WaitInvokeRetUnified<TRet>(proc_name, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvoke(const std::string& proc_name, const TArgs&... args) const
{
	std::vector<uint8_t> result_buf;
	InvokeUnified<void>(proc_name, &result_buf, false, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(const std::string& proc_name, const TArgs&... args) const
{
	InvokeUnified<void>(proc_name, nullptr, false, args...);
}

template<class... TArgs>
void MAssIpcCall::WaitInvokeAlertable(const std::string& proc_name, const TArgs&... args) const
{
	std::vector<uint8_t> result_buf;
	InvokeUnified<void>(proc_name, &result_buf, true, args...);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRetAlertable(const std::string& proc_name, const TArgs&... args) const
{
	return WaitInvokeRetUnified<TRet>(proc_name, true, args...);
}

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
