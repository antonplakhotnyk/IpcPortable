#include "MAssIpc_Internals.h"
#include "MAssIpc_Macros.h"


namespace MAssIpcCallInternal
{

static std::unique_ptr<MAssIpc_Data> CreateDataBuffer(const std::weak_ptr<MAssIpc_TransportShare>& weak_transport, MAssIpc_Data::TPacketSize packet_size)
{
	auto transport = weak_transport.lock();
	mass_return_x_if_equal(bool(transport), false, {});
	return transport->Create(packet_size);
}

MAssIpc_DataStream CreateDataStream(const std::weak_ptr<MAssIpc_TransportShare>& weak_transport, 
														 MAssIpc_Data::TPacketSize no_header_size, 
														 MAssIpc_PacketParser::PacketType pt, 
														 MAssIpc_PacketParser::TCallId respond_id)
{
	if( respond_id!=MAssIpc_PacketParser::c_invalid_id )
	{
		auto data_buffer = CreateDataBuffer(weak_transport, MAssIpc_PacketParser::c_net_call_packet_header_size+no_header_size);
		if( data_buffer )
		{
			MAssIpc_DataStream result(std::move(data_buffer));
			MAssIpc_PacketParser::PacketHeaderWrite(result, no_header_size, pt, respond_id);
			return result;
		}
	}

	return {};
}


MAssIpc_DataStream CreateDataStreamInplace(std::unique_ptr<MAssIpc_Data> inplace_send_buffer,
											  MAssIpc_Data::TPacketSize no_header_size,
											  MAssIpc_PacketParser::PacketType pt,
											  MAssIpc_PacketParser::TCallId respond_id)
{
	MAssIpc_Data::TPacketSize packet_size = no_header_size+MAssIpc_PacketParser::c_net_call_packet_header_size;
	mass_return_x_if_equal(inplace_send_buffer->Size() < packet_size, true, {});
	MAssIpc_DataStream result(std::move(inplace_send_buffer));
	MAssIpcCallInternal::MAssIpc_PacketParser::PacketHeaderWrite(result, no_header_size, MAssIpcCallInternal::MAssIpc_PacketParser::PacketType::pt_call, respond_id);
	return result;
}

std::unique_ptr<const MAssIpc_Data> SerializeReturn(const std::weak_ptr<MAssIpc_TransportShare>& transport,
												   MAssIpc_PacketParser::TCallId respond_id)
{
	if( respond_id == MAssIpc_PacketParser::c_invalid_id )
		return {};
	MAssIpc_DataStream result_stream(CreateDataStream(transport, 0, MAssIpc_PacketParser::PacketType::pt_return, respond_id));
	return result_stream.DetachWrite();
}

//-------------------------------------------------------

void ResultJob::Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, std::unique_ptr<const MAssIpc_Data>& result)
{
	mass_return_if_equal(result->Size()>=MAssIpc_PacketParser::c_net_call_packet_header_size, false);
	auto transport_strong = transport.lock();
	mass_return_if_equal(bool(transport_strong), false);
	transport_strong->Write(std::move(result));
}

//-------------------------------------------------------

MAssIpc_PacketParser::TCallId CallJob::CalcRespondId(bool send_return, MAssIpc_PacketParser::TCallId id)
{
	return (send_return ? id : MAssIpc_PacketParser::c_invalid_id);
}

void CallJob::Invoke()
{
	Invoke(m_transport, m_inter_thread, m_call_info_data, m_invoke_base, m_respond_id);
}

void CallJob::Invoke(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread,
					 MAssIpc_DataStream& call_info_data, std::shared_ptr<const InvokeRemoteBase> invoke_base,
					 MAssIpc_PacketParser::TCallId respond_id)
{
	auto result = invoke_base->Invoke(transport, respond_id, call_info_data);
	
	if( respond_id!=MAssIpc_PacketParser::c_invalid_id )
	{
		if( auto inter_thread_strong = inter_thread.lock() )
		{
			auto result_thread_id = inter_thread_strong->GetResultSendThreadId();
			std::unique_ptr<ResultJob> result_job(std::make_unique<ResultJob>(transport, result));
			inter_thread_strong->CallFromThread(result_thread_id, std::move(result_job));
		}
		else
			ResultJob::Invoke(transport, result);
	}
}

//-------------------------------------------------------

ProcMap::FindCallInfoRes ProcMap::FindCallInfo(const MAssIpc_RawString& proc_name, const MAssIpc_RawString& params_type) const
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);

	const CallInfoImpl::SignatureKey key{proc_name,params_type};
	auto it_procs_signature = m_name_procs.find(key);
	if( it_procs_signature==m_name_procs.end() )
		return {{},{},{},m_OnInvalidRemoteCall};

	std::shared_ptr<InvokeRemoteBase> invoke_base = it_procs_signature->second.call_info->IsInvokable();
	return {it_procs_signature->second.call_info, invoke_base, m_OnCallCountChanged};
}

MAssIpcCall_EnumerateData ProcMap::EnumerateHandlers() const
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);

	MAssIpcCall_EnumerateData res;

	for( const auto& it : m_name_procs)
		if( auto invoke_base = it.second.call_info->IsInvokable() )
			res.push_back({it.first.name.Std_String(), invoke_base->GetSignature_RetType().Std_String(), it.first.params_type.Std_String(), it.second.comment});

	return res;
}

std::shared_ptr<const CallInfo> ProcMap::AddProcSignature(const MAssIpc_RawString& proc_name, std::string params_type, 
														  std::unique_ptr<InvokeRemoteBase> invoke, const std::string& comment)
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
	std::shared_ptr<CallInfoImpl> result_call_info;

	auto it_name_params = m_name_procs.find(CallInfoImpl::SignatureKey{proc_name,params_type});
	if( it_name_params==m_name_procs.end() )
	{
		result_call_info = std::make_shared<CallInfoImpl>(proc_name, std::move(params_type));
		m_name_procs.emplace(result_call_info->GetSignatureKey(), CallData{result_call_info, comment});
	}
	else
	{
		mass_return_x_if_equal(bool(it_name_params->second.call_info->IsInvokable()), true, {});
		it_name_params->second = CallData{it_name_params->second.call_info, comment};
		result_call_info = it_name_params->second.call_info;
	}

	result_call_info->SetInvoke(std::move(invoke));
	return result_call_info;
}

void ProcMap::AddAllProcs(const ProcMap& other)
{
	if( this == &other )
		return;

	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock_other(other.m_lock);

	for( auto other_it : other.m_name_procs )
		m_name_procs.insert(other_it);
}

void ProcMap::ClearAllProcs()
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
	m_name_procs.clear();
}

void ProcMap::ClearProcsWithTag(const void* tag)
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);

	for( auto it = m_name_procs.begin(); it!=m_name_procs.end(); )
	{
		if( auto invoke_base = it->second.call_info->IsInvokable() )
			if( invoke_base->m_tag == tag )
			{
				it = m_name_procs.erase(it);
				continue;
			}
		it++;
	}

	ClearCallbackTag(&m_OnCallCountChanged, tag);
	ClearCallbackTag(&m_OnInvalidRemoteCall, tag);
}

std::shared_ptr<const CallCountChanged> ProcMap::SetCallCountChanged(std::shared_ptr<const CallCountChanged> new_val)
{
	return SetCallback(&m_OnCallCountChanged, new_val);
}

std::shared_ptr<const ErrorOccured> ProcMap::SetErrorHandler(std::shared_ptr<const ErrorOccured> new_val)
{
	return SetCallback(&m_OnInvalidRemoteCall, new_val);
}

std::shared_ptr<const ErrorOccured> ProcMap::GetErrorHandler() const
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_lock);
	return m_OnInvalidRemoteCall;
}

}// namespace MAssIpcCallInternal;


