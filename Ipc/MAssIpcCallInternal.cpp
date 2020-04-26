#include "MAssIpcCallInternal.h"
#include "Integration/MAssMacros.h"


namespace MAssIpcCallInternal
{

//-------------------------------------------------------
CallInfo::CallInfo(MAssThread::Id thread_id)
	:m_thread_id(thread_id)
{
};

//-------------------------------------------------------

ResultJob::ResultJob(const std::shared_ptr<MAssIpcCallTransport>& transport)
	:m_transport(transport)
{
	MAssIpcCallPacket::PacketHeaderAllocate(&m_result, MAssIpcCallPacket::pt_return);
};

void ResultJob::Invoke()
{
	mass_return_if_equal(m_result.size()>=MAssIpcCallPacket::c_net_call_packet_header_size, false);
	MAssIpcCallPacket::PacketHeaderUpdateSize(&m_result);
	m_transport->Write(m_result.data(), m_result.size());
}

//-------------------------------------------------------

CallJob::CallJob(const std::shared_ptr<MAssIpcCallTransport>& transport, const std::shared_ptr<MAssCallThreadTransport>& inter_thread,
				 std::unique_ptr<std::vector<uint8_t> > call_info_data)
	: m_call_info_data_str(call_info_data->data(), call_info_data->size())
	, m_call_info_data(std::move(call_info_data))
	, m_result_thread_id(inter_thread ? inter_thread->GetCurrentThreadId() : MAssThread::c_no_thread)
	, m_transport(transport)
	, m_inter_thread(inter_thread)
{
}

void CallJob::Invoke()
{
	std::shared_ptr<ResultJob> result_job(new ResultJob(m_transport));
	MAssIpcCallDataStream result_str(&result_job->m_result);
	m_call_info->Invoke(&result_str, &m_call_info_data_str);
	if( m_send_return )
	{
		if( m_inter_thread )
			m_inter_thread->CallFromThread(m_result_thread_id, result_job);
		else
			result_job->Invoke();
	}
}

//-------------------------------------------------------

void ProcMap::FindCallInfo(const std::string& name, std::string& signature, std::shared_ptr<const CallInfo>* call_info) const
{
	auto it_procs = m_name_procs.find(name);
	if( it_procs==m_name_procs.end() )
		return;

	auto it_signature = it_procs->second.m_signature_call.find(signature);
	if( it_signature==it_procs->second.m_signature_call.end() )
		return;

	*call_info = it_signature->second.call_info;
}

MAssIpcCall_EnumerateData ProcMap::EnumerateHandlers()
{
	MAssIpcCall_EnumerateData res;

	for( auto it_np = m_name_procs.begin(); it_np!=m_name_procs.end(); it_np++ )
		for( auto it_sc = it_np->second.m_signature_call.begin(); it_sc!=it_np->second.m_signature_call.end(); it_sc++ )
			res.push_back({it_np->first, it_sc->second.call_info->GetSignature_RetType(), it_sc->first, it_sc->second.comment});

	return res;
}



}// namespace MAssIpcCallInternal;

