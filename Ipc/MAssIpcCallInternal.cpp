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
			return std::move(result);
		}
	}

	return {};
}


MAssIpcCallDataStream CreateDataStreamInplace(std::unique_ptr<MAssIpcData>& inplace_send_buffer,
											  MAssIpcData::TPacketSize no_header_size,
											  MAssIpcPacketParser::PacketType pt,
											  MAssIpcPacketParser::TCallId respond_id)
{
	MAssIpcData::TPacketSize packet_size = no_header_size+MAssIpcPacketParser::c_net_call_packet_header_size;
	mass_return_x_if_equal(inplace_send_buffer->Size() < packet_size, true, {});
	MAssIpcCallDataStream result(std::move(inplace_send_buffer));
	MAssIpcCallInternal::MAssIpcPacketParser::PacketHeaderWrite(result, no_header_size, MAssIpcCallInternal::MAssIpcPacketParser::PacketType::pt_call, respond_id);
	return std::move(result);
}

//-------------------------------------------------------
CallInfo::CallInfo(MAssIpcThreadTransportTarget::Id thread_id)
	:m_thread_id(thread_id)
{
};

//-------------------------------------------------------

ResultJob::ResultJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, std::unique_ptr<MAssIpcData>& result)
	:m_transport(transport)
	, m_result(std::move(result))
{
};

void ResultJob::Invoke()
{
	mass_return_if_equal(m_result->Size()>=MAssIpcPacketParser::c_net_call_packet_header_size, false);
	auto transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);
	transport->Write(std::move(m_result));
}

//-------------------------------------------------------

CallJob::CallJob(const std::weak_ptr<MAssIpcPacketTransport>& transport, const std::weak_ptr<MAssCallThreadTransport>& inter_thread,
				 MAssIpcCallDataStream& call_info_data, MAssIpcPacketParser::TCallId id)
	: m_call_info_data_str(std::move(call_info_data))
	, m_result_thread_id(MakeResultThreadId(inter_thread))
	, m_transport(transport)
	, m_inter_thread(inter_thread)
	, m_id(id)
{
}

MAssIpcThreadTransportTarget::Id CallJob::MakeResultThreadId(const std::weak_ptr<MAssCallThreadTransport>& inter_thread)
{
	auto inter_thread_strong = inter_thread.lock();
	return inter_thread_strong ? inter_thread_strong->GetResultSendThreadId() : MAssCallThreadTransport::NoThread();
}

void CallJob::Invoke()
{
	auto respond_id = (m_send_return ? m_id : MAssIpcPacketParser::c_invalid_id);
	auto result = m_call_info->Invoke(m_transport, respond_id, m_call_info_data_str);
	if( m_send_return )
	{
		std::unique_ptr<ResultJob> result_job(std::make_unique<ResultJob>(m_transport, result));

		auto inter_thread = m_inter_thread.lock();
		if( inter_thread )
			inter_thread->CallFromThread(m_result_thread_id, std::move(result_job));
		else
			result_job->Invoke();
	}
}

//-------------------------------------------------------

std::shared_ptr<const CallInfo> ProcMap::FindCallInfo(const std::string& name, std::string& signature) const
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	auto it_procs = m_name_procs.find(name);
	if( it_procs==m_name_procs.end() )
		return {};

	auto it_signature = it_procs->second.m_signature_call.find(signature);
	if( it_signature==it_procs->second.m_signature_call.end() )
		return {};

	return it_signature->second.call_info;
}

MAssIpcCall_EnumerateData ProcMap::EnumerateHandlers() const
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	MAssIpcCall_EnumerateData res;

	for( auto it_np = m_name_procs.begin(); it_np!=m_name_procs.end(); it_np++ )
		for( auto it_sc = it_np->second.m_signature_call.begin(); it_sc!=it_np->second.m_signature_call.end(); it_sc++ )
			res.push_back({it_np->first, it_sc->second.call_info->GetSignature_RetType(), it_sc->first, it_sc->second.comment});

	return res;
}

void ProcMap::AddProcSignature(const std::string& proc_name, std::string& params_type,
								   const std::shared_ptr<MAssIpcCallInternal::CallInfo>& new_call_info, const std::string& comment)
{
	MAssIpcThreadSafe::unique_lock<MAssIpcThreadSafe::mutex> lock(m_lock);

	std::shared_ptr<MAssIpcCallInternal::CallInfo> ownership_call_info(new_call_info);
	auto it_name = m_name_procs.find(proc_name);
	if( it_name == m_name_procs.end() )
	{
		m_name_procs[proc_name] = MAssIpcCallInternal::ProcMap::NameProcs();
		it_name = m_name_procs.find(proc_name);
	}

	auto it_params = it_name->second.m_signature_call.find(params_type);
	mass_return_if_equal((it_params!=it_name->second.m_signature_call.end()) && (it_params->second.call_info->IsCallable()), true);
	it_name->second.m_signature_call[params_type] = {ownership_call_info,comment};
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


}// namespace MAssIpcCallInternal;


