#pragma once

#include "MAssIpcCallDataStream.h"
#include <functional>
#include <memory>
#include "MAssIpcCallPacket.h"
#include "MAssCallThreadTransport.h"
#include "MAssIpcCallInternal.h"


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
	MAssIpcCall(const std::shared_ptr<MAssCallThreadTransport>& inter_thread_nullable);

	void SetTransport(const std::shared_ptr<MAssIpcCallTransport>& transport);

	template<class TRet, class... TArgs>
	TRet WaitInvokeRet(const std::string& proc_name, TArgs&... args) const;
	template<class... TArgs>
	void WaitInvoke(const std::string& proc_name, TArgs&... args) const;

	template<class... TArgs>
	void AsyncInvoke(const std::string& proc_name, TArgs&... args) const;


	template<class TDelegateW>
	void AddHandler(const std::string& proc_name, const TDelegateW& del,
					MAssThread::Id thread_id = MAssThread::c_no_thread);
	template<class TDelegateW>
	void AddHandler(const std::string& proc_name, const TDelegateW& del, const std::string& comment,
					MAssThread::Id thread_id = MAssThread::c_no_thread);

	void SetErrorHandler(TErrorHandler OnInvalidRemoteCall);

// 	template<class TFunc>
// 	void AddHandler2(const std::string& proc_name, const TFunc& del, const std::string& comment = std::string(),
// 					 MAssThread::Id thread_id = MAssThread::c_no_thread);

	void ProcessTransport();

	MAssIpcCall_EnumerateData EnumerateRemote() const;
	MAssIpcCall_EnumerateData EnumerateLocal() const;

	static constexpr char separator = MAssIpcCallInternal::separator;

private:

	static void SerializeCallSignature(MAssIpcCallDataStream* call_info, const std::string& proc_name,
									   bool send_return, const std::string& return_type,
									   const std::string& params_type);

	template<class TRet, class... TArgs>
	void InvokeUnified(const std::string& proc_name, std::vector<uint8_t>* result_buf_wait_return, TArgs&... args) const;
	void InvokeRemote(const std::vector<uint8_t>& call_info_data, std::vector<uint8_t>* result) const;

	void AddProcSignature(const std::string& proc_name, std::string& params_type, const std::shared_ptr<MAssIpcCallInternal::CallInfo>& call_info, const std::string& comment);

	static void DeserializeNameSignature(MAssIpcCallDataStream* call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type);

private:

	class Internals
	{
	public:

		void ProcessTransportExternal();
		size_t ProcessTransport(std::vector<uint8_t>* result);

		MAssIpcCallInternal::ProcMap				m_proc_map;
		MAssIpcCallInternal::MAssIpcCallPacket		m_packet_parser;
		std::shared_ptr<MAssIpcCallTransport>		m_transport;
		std::shared_ptr<MAssCallThreadTransport>	m_inter_thread_nullable;
		TErrorHandler								m_OnInvalidRemoteCall;

	private:

		void InvokeLocal(std::unique_ptr<std::vector<uint8_t> > call_info_data) const;

		struct CreateCallJobRes
		{
			std::shared_ptr<MAssIpcCallInternal::CallJob> obj;
			bool send_return = false;
			ErrorType error = ErrorType::unknown_error;
			std::string message;
		};

		CreateCallJobRes CreateCallJob(std::unique_ptr<std::vector<uint8_t> > call_info_data) const;

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
	AddProcSignature(proc_name, params_type, call_info, comment);
}

template<class TRet, class... TArgs>
void MAssIpcCall::InvokeUnified(const std::string& proc_name, std::vector<uint8_t>* result_buf_wait_return, TArgs&... args) const
{
	typedef TRet(*TreatProc)(TArgs...);
	std::string return_type;
	std::string params_type;
	return_type = MAssIpcType<TRet>::NameValue();
	MAssIpcCallInternal::ProcSignature<TreatProc>::GetParams(&params_type);

// 	int unpack[]{0,((MAssIpcCallInternal::CheckArgType<TArgs>()),0)...};
// 	unpack;


	std::vector<uint8_t> call_info_data_buf;
	MAssIpcCallDataStream call_info(&call_info_data_buf);
	{
		MAssIpcCallInternal::MAssIpcCallPacket::PacketHeaderAllocate(&call_info_data_buf, MAssIpcCallInternal::MAssIpcCallPacket::pt_call);
		MAssIpcCall::SerializeCallSignature(&call_info, proc_name, (result_buf_wait_return!=nullptr), return_type, params_type);
		MAssIpcCallInternal::SerializeArgs(&call_info, args...);
		MAssIpcCallInternal::MAssIpcCallPacket::PacketHeaderUpdateSize(&call_info_data_buf);
	}

	InvokeRemote(call_info_data_buf, result_buf_wait_return);
}

template<class TRet, class... TArgs>
TRet MAssIpcCall::WaitInvokeRet(const std::string& proc_name, TArgs&... args) const
{
	static_assert(!std::is_same<TRet,void>::value, "can not be implicit void use function call explicitely return void");

	std::vector<uint8_t> result_buf;
	InvokeUnified<TRet>(proc_name, &result_buf, args...);
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

template<class... TArgs>
void MAssIpcCall::WaitInvoke(const std::string& proc_name, TArgs&... args) const
{
	std::vector<uint8_t> result_buf;
	InvokeUnified<void>(proc_name, &result_buf, args...);
}

template<class... TArgs>
void MAssIpcCall::AsyncInvoke(const std::string& proc_name, TArgs&... args) const
{
	InvokeUnified<void>(proc_name, nullptr, args...);
}

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall::ErrorType& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall::ErrorType& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall::ErrorType);
