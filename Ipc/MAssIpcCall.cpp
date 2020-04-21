#include "MAssIpcCall.h"
#include "Integration/MAssMacros.h"


MAssIpcCall::MAssIpcCall(const std::shared_ptr<MAssCallThreadTransport>& inter_thread_nullable)
	:m_int(new Internals)
{
	m_int->m_inter_thread_nullable = inter_thread_nullable;
}

void MAssIpcCall::SetTransport(const std::shared_ptr<MAssIpcCallTransport>& transport)
{
	m_int->m_packet_parser = MAssIpcCallPacket();
	m_int->m_transport = transport;
}

void MAssIpcCall::DeserializeNameSignature(MAssIpcCallDataStream* call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type)
{
	(*call_info)>>(*proc_name)>>(*send_return)>>(*return_type)>>(*params_type);
}

void MAssIpcCall::Internals::InvokeLocal(std::unique_ptr<std::vector<uint8_t> > call_info_data) const
{
	std::shared_ptr<MAssIpcCallInternal::CallJob> call_job(new MAssIpcCallInternal::CallJob(m_transport,m_inter_thread_nullable,std::move(call_info_data)));

	{
		std::string return_type;
		std::string params_type;
		std::string proc_name;

		DeserializeNameSignature(&call_job->m_call_info_data_str, &proc_name, &call_job->m_send_return, &return_type, &params_type);

		m_proc_map.FindCallInfo(proc_name, params_type, &call_job->m_call_info);
		if( !bool(call_job->m_call_info) )
		{
			if( m_OnInvalidRemoteCall )
				m_OnInvalidRemoteCall(ErrorType::no_matching_call_name_parameters, return_type+" "+proc_name+"("+params_type+")");
			return;
		}

		if( !return_type.empty() )
		{
			const std::string& return_type_call = call_job->m_call_info->GetSignature_RetType();
			if( return_type != return_type_call )
			{
				if( m_OnInvalidRemoteCall )
					m_OnInvalidRemoteCall(ErrorType::no_matching_call_return_type, return_type+" "+proc_name+"("+params_type+")");
				return;
			}
		}
	}

	if( m_inter_thread_nullable )
		m_inter_thread_nullable->CallFromThread(call_job->m_call_info->ThreadId(), call_job);
	else
		call_job->Invoke();

// 	{
// 		if( *send_return )
// 		{
// 			MAssIpcCallDataStream result_str(result);
// 			call_info->Invoke(&result_str, &call_info_data_str);
// 		}
// 		else
// 			call_info->Invoke(NULL, &call_info_data_str);
// 	}
}

void MAssIpcCall::InvokeRemote(const std::vector<uint8_t>& call_info_data, std::vector<uint8_t>* result, MAssIpcCallPacket::PacketType type) const
{
	mass_return_if_equal(bool(m_int->m_transport), false);
	MAssIpcCallPacket::SendData(call_info_data, type, m_int->m_transport);

	if(result)
	{
		while( true )
		{
			size_t return_processed_need_more_data_size = m_int->ProcessTransport(result);
			if( return_processed_need_more_data_size>0 )
				m_int->m_transport->WaitRespond(return_processed_need_more_data_size);
			else
				break;
		}
	}
}

void MAssIpcCall::Internals::ProcessTransportExternal()
{
	ProcessTransport(nullptr);
}

size_t MAssIpcCall::Internals::ProcessTransport(std::vector<uint8_t>* result)
{
	size_t return_processed_need_more_data_size = 0;
	mass_return_x_if_equal(bool(m_transport), false, return_processed_need_more_data_size);

	while( true )
	{
		return_processed_need_more_data_size = m_packet_parser.IsNeedMoreDataSize(m_transport);
		if( return_processed_need_more_data_size>0 )
			break;

		MAssIpcCallPacket::PacketType data_type = m_packet_parser.ReadType(m_transport);

		switch( data_type )
		{
			case MAssIpcCallPacket::pt_call:
			{
				std::unique_ptr<std::vector<uint8_t> > in_data(new std::vector<uint8_t>);
				m_packet_parser.ReadData(m_transport, in_data.get());
				InvokeLocal(std::move(in_data));
			}
			break;
			case MAssIpcCallPacket::pt_enumerate_return:
			case MAssIpcCallPacket::pt_return:
			{
				mass_return_x_if_equal(result, nullptr, 0);
				m_packet_parser.ReadData(m_transport, result);
				return 0;
			}
			break;
			case MAssIpcCallPacket::pt_enumerate:
			{
				std::vector<uint8_t> result_now;
				MAssIpcCall_EnumerateData res = m_proc_map.EnumerateHandlers();
				MAssIpcCall::PacketHeaderAllocate(&result_now, MAssIpcCallPacket::pt_enumerate_return);
				{
					MAssIpcCallDataStream data_raw(&result_now);
					data_raw<<res;
				}
				MAssIpcCall::PacketHeaderUpdateSize(&result_now);
				m_transport->Write(result_now.data(), result_now.size());
			}
			break;
			default:
				break;
		}
	}

	return return_processed_need_more_data_size;
}

void MAssIpcCall::ProcessTransport()
{
	m_int->ProcessTransportExternal();
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateRemote() const
{
	std::vector<uint8_t> call_info_data;
	std::vector<uint8_t> result;

	MAssIpcCall::PacketHeaderAllocate(&call_info_data, MAssIpcCallPacket::pt_enumerate);
	MAssIpcCall::PacketHeaderUpdateSize(&call_info_data);

	InvokeRemote(call_info_data, &result, MAssIpcCallPacket::pt_enumerate);

	MAssIpcCall_EnumerateData res;
	MAssIpcCallDataStream data_raw(&result);
	data_raw>>res;

	return res;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateLocal() const
{
	return m_int->m_proc_map.EnumerateHandlers();
}

void MAssIpcCall::AddProcSignature(const std::string& proc_name, std::string& params_type,
							   const std::shared_ptr<MAssIpcCallInternal::CallInfo>& new_call_info, const std::string& comment)
{
	std::shared_ptr<MAssIpcCallInternal::CallInfo> ownership_call_info(new_call_info);
	auto it_name = m_int->m_proc_map.m_name_procs.find(proc_name);
	if( it_name == m_int->m_proc_map.m_name_procs.end() )
	{
		m_int->m_proc_map.m_name_procs[proc_name] = MAssIpcCallInternal::ProcMap::NameProcs();
		it_name = m_int->m_proc_map.m_name_procs.find(proc_name);
	}

	auto it_params = it_name->second.m_signature_call.find(params_type);
	mass_return_if_equal((it_params!=it_name->second.m_signature_call.end()) && (it_params->second.call_info->IsCallable()), true );
	it_name->second.m_signature_call[params_type] = {ownership_call_info,comment};
}

void MAssIpcCall::SerializeCallSignature(MAssIpcCallDataStream* call_info, const std::string& proc_name,
									 bool send_return, const std::string& return_type,
									 const std::string& params_type)
{
	(*call_info)<<proc_name<<send_return<<return_type<<params_type;
}

void MAssIpcCall::PacketHeaderAllocate(std::vector<uint8_t>* packet_data, MAssIpcCallPacket::PacketType pt)
{
	MAssIpcCallDataStream out_data_raw(packet_data);
	out_data_raw<<uint32_t(0)<<uint32_t(pt);
}

void MAssIpcCall::PacketHeaderUpdateSize(std::vector<uint8_t>* packet_data)
{
	mass_return_if_equal(packet_data->size()<MAssIpcCallPacket::c_net_call_packet_header_size, true);
	uint32_t* packet_data_size = reinterpret_cast<uint32_t*>(packet_data->data());
	*packet_data_size = packet_data->size() - MAssIpcCallPacket::c_net_call_packet_header_size;
}

char MAssIpcCall::GetTypeNameSeparator()
{
	return MAssIpcCallInternal::separator;
}

void MAssIpcCall::SetErrorHandler(TErrorHandler OnInvalidRemoteCall)
{
	m_int->m_OnInvalidRemoteCall = OnInvalidRemoteCall;
}

//-------------------------------------------------------
