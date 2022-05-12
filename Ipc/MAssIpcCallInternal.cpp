#include "MAssIpcCallInternal.h"
#include "MAssMacros.h"


namespace MAssIpcCallInternal
{

static std::unique_ptr<MAssIpcData> CreateDataBuffer(const std::weak_ptr<MAssIpcPacketTransport>& weak_transport, MAssIpcData::TPacketSize packet_size)
{
	auto transport = weak_transport.lock();
	mass_return_x_if_equal(bool(transport), false, {});
	return transport->Create(packet_size);
}

MAssIpcCallDataStream CreateDataStream(const std::weak_ptr<MAssIpcPacketTransport>& weak_transport, 
														 MAssIpcData::TPacketSize no_header_size, 
														 MAssIpcPacketParser::PacketType pt, 
														 MAssIpcPacketParser::TCallId respond_id)
{
	if( respond_id!=MAssIpcPacketParser::c_invalid_id )
	{
		auto data_buffer = CreateDataBuffer(weak_transport, MAssIpcPacketParser::c_net_call_packet_header_size+no_header_size);
		if( data_buffer )
		{
			MAssIpcCallDataStream result(std::move(data_buffer));
			MAssIpcPacketParser::PacketHeaderWrite(result, no_header_size, pt, respond_id);
			return result;
		}
	}

	return {};
}


MAssIpcCallDataStream CreateDataStreamInplace(std::unique_ptr<MAssIpcData> inplace_send_buffer,
											  MAssIpcData::TPacketSize no_header_size,
											  MAssIpcPacketParser::PacketType pt,
											  MAssIpcPacketParser::TCallId respond_id)
{
	MAssIpcData::TPacketSize packet_size = no_header_size+MAssIpcPacketParser::c_net_call_packet_header_size;
	mass_return_x_if_equal(inplace_send_buffer->Size() < packet_size, true, {});
	MAssIpcCallDataStream result(std::move(inplace_send_buffer));
	MAssIpcCallInternal::MAssIpcPacketParser::PacketHeaderWrite(result, no_header_size, MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, respond_id);
	return result;
}

std::unique_ptr<const MAssIpcData> SerializeReturn(const std::weak_ptr<MAssIpcPacketTransport>& transport,
												   MAssIpcPacketParser::TCallId respond_id)
{
	if( respond_id == MAssIpcPacketParser::c_invalid_id )
		return {};
	MAssIpcCallDataStream result_stream(CreateDataStream(transport, 0, MAssIpcPacketParser::PacketType::pt_return, respond_id));
	return result_stream.DetachWrite();
}

//-------------------------------------------------------
CallInfo::CallInfo(MAssIpcThreadTransportTarget::Id thread_id, const MAssIpcRawString& proc_name, std::string params_type)
	:m_thread_id(thread_id)
	, m_name(proc_name.Std_String())
	, m_params_type(std::move(params_type))
{
};
//-------------------------------------------------------

ResultJob::ResultJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<const MAssIpcData>& result)
	:m_transport(transport)
	, m_result(std::move(result))
{
};

void ResultJob::Invoke()
{
	Invoke(m_transport, m_result);
}

void ResultJob::Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<const MAssIpcData>& result)
{
	mass_return_if_equal(result->Size()>=MAssIpcPacketParser::c_net_call_packet_header_size, false);
	auto transport_strong = transport.lock();
	mass_return_if_equal(bool(transport_strong), false);
	transport_strong->Write(std::move(result));
}

//-------------------------------------------------------

CallJob::CallJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, 
				 const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
				 MAssIpcCallDataStream& call_info_data, MAssIpcPacketParser::TCallId respond_id,
				 std::shared_ptr<const CallInfo> call_info)
	: m_call_info_data(std::move(call_info_data))
	, m_transport(transport)
	, m_inter_thread(inter_thread)
	, m_respond_id(respond_id)
	, m_call_info(call_info)
{
}

MAssIpcPacketParser::TCallId CallJob::CalcRespondId(bool send_return, MAssIpcPacketParser::TCallId id)
{
	return (send_return ? id : MAssIpcPacketParser::c_invalid_id);
}

void CallJob::Invoke()
{
	Invoke(m_transport, m_inter_thread, m_call_info_data, m_call_info, m_respond_id);
}

void CallJob::Invoke(const std::weak_ptr<MAssIpcPacketTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
					 MAssIpcCallDataStream& call_info_data, std::shared_ptr<const CallInfo> call_info, 
					 MAssIpcPacketParser::TCallId respond_id)
{
	auto result = call_info->Invoke(transport, respond_id, call_info_data);
	if( respond_id!=MAssIpcPacketParser::c_invalid_id )
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

std::shared_ptr<const CallInfo> ProcMap::FindCallInfo(const MAssIpcRawString& name, const MAssIpcRawString& params_type) const
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	const CallInfo::SignatureKey key{name,params_type};
	auto it_procs_signature = m_name_procs.find(key);
	if( it_procs_signature==m_name_procs.end() )
		return {};

	return it_procs_signature->second.call_info;
}

MAssIpcCall_EnumerateData ProcMap::EnumerateHandlers() const
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	MAssIpcCall_EnumerateData res;

	for( const auto& it : m_name_procs)
		res.push_back({it.first.name.Std_String(), it.second.call_info->GetSignature_RetType().Std_String(), it.first.params_type.Std_String(), it.second.comment});

	return res;
}

void ProcMap::AddProcSignature(const std::shared_ptr<MAssIpcCallInternal::CallInfo>& new_call_info, const std::string& comment, const void* tag)
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	std::shared_ptr<MAssIpcCallInternal::CallInfo> ownership_call_info(new_call_info);

	auto key{new_call_info->GetSignatureKey()};
	auto it_name_params = m_name_procs.find(key);
	mass_return_if_equal((it_name_params!=m_name_procs.end()) && (it_name_params->second.call_info->IsCallable()), true);
	m_name_procs[key] = {ownership_call_info, comment, tag};
}

void ProcMap::AddAllProcs(const ProcMap& other)
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock_other(other.m_lock);

	for( auto other_it : other.m_name_procs )
		m_name_procs.insert(other_it);
}

void ProcMap::ClearAllProcs()
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);
	m_name_procs.clear();
}

void ProcMap::ClearProcsWithTag(const void* tag)
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	for( auto it = m_name_procs.begin(); it!=m_name_procs.end(); )
		if( it->second.tag == tag )
			it = m_name_procs.erase(it);
		else
			it++;
}



}// namespace MAssIpcCallInternal;


