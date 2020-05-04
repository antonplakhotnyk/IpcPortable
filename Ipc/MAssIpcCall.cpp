#include "MAssIpcCall.h"
#include "Integration/MAssMacros.h"

using namespace MAssIpcCallInternal;

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssCallThreadTransport>& inter_thread_nullable)
	:m_int(new Internals)
{
	m_int->m_inter_thread_nullable = inter_thread_nullable;
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpcCallTransport>& transport)
{
	m_int->m_packet_parser = MAssIpcCallPacket();
	m_int->m_transport = transport;
}

void MAssIpcCall::DeserializeNameSignature(MAssIpcCallDataStream* call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type)
{
	(*call_info)>>(*proc_name)>>(*send_return)>>(*return_type)>>(*params_type);
}

MAssIpcCall::Internals::CreateCallJobRes MAssIpcCall::Internals::CreateCallJob(const std::shared_ptr<MAssIpcCallTransport>& transport, std::unique_ptr<std::vector<uint8_t> > call_info_data) const
{
	CreateCallJobRes res = {};
	std::shared_ptr<MAssIpcCallInternal::CallJob> call_job(new MAssIpcCallInternal::CallJob(transport, m_inter_thread_nullable, std::move(call_info_data)));

	{
		std::string return_type;
		std::string params_type;
		std::string proc_name;

		DeserializeNameSignature(&call_job->m_call_info_data_str, &proc_name, &call_job->m_send_return, &return_type, &params_type);
		res.send_return = call_job->m_send_return;

		call_job->m_call_info = m_proc_map.FindCallInfo(proc_name, params_type);
		if( !bool(call_job->m_call_info) )
		{
			res.message = return_type+" "+proc_name+"("+params_type+")";
			res.error = ErrorType::no_matching_call_name_parameters;
			if( m_OnInvalidRemoteCall )
				m_OnInvalidRemoteCall(res.error, res.message);
			return res;
		}

		if( !return_type.empty() )
		{
			const std::string& return_type_call = call_job->m_call_info->GetSignature_RetType();
			if( return_type != return_type_call )
			{
				res.message = return_type+" "+proc_name+"("+params_type+")";
				res.error = ErrorType::no_matching_call_return_type;
				if( m_OnInvalidRemoteCall )
					m_OnInvalidRemoteCall(res.error, res.message);
				return res;
			}
		}
	}

	res.obj = call_job;
	return res;
}

void MAssIpcCall::Internals::InvokeLocal(std::unique_ptr<std::vector<uint8_t> > call_info_data) const
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	CreateCallJobRes call_job = CreateCallJob(transport, std::move(call_info_data));

	if(call_job.obj)
	{
		auto inter_thread_nullable = m_inter_thread_nullable.lock();
		if( inter_thread_nullable )
			inter_thread_nullable->CallFromThread(call_job.obj->m_call_info->m_thread_id, call_job.obj);
		else
			call_job.obj->Invoke();
	}
	else
	{// fail to call
		if( call_job.send_return )
		{
			std::vector<uint8_t> result;
			MAssIpcCallDataStream result_str(&result);
			MAssIpcCallPacket::PacketHeaderAllocate(&result, MAssIpcCallPacket::pt_return_fail_call);
			result_str<<call_job.error;
			result_str<<call_job.message;
			MAssIpcCallPacket::PacketHeaderUpdateSize(&result);
			transport->Write(result.data(), result.size());
		}
	}
}

void MAssIpcCall::ProcessTransport()
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_int->m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	while( true )
	{
		size_t needed_data_size = m_int->TryProcessSingleCall(transport, nullptr);
		if( needed_data_size>0 )
			break;
	}
}

void MAssIpcCall::InvokeRemote(const std::vector<uint8_t>& call_info_data, std::vector<uint8_t>* result) const
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_int->m_transport.lock();
	mass_return_if_equal(bool(transport), false);
	transport->Write(call_info_data.data(), call_info_data.size());

	if(result)
	{
		while( true )
		{
			size_t needed_data_size = m_int->TryProcessSingleCall(transport, result);
			if( needed_data_size>0 )
			{
				if( !transport->WaitRespond(needed_data_size) )
					break;
			}
			else
				break;
		}
	}
}

size_t MAssIpcCall::Internals::TryProcessSingleCall(const std::shared_ptr<MAssIpcCallTransport>& transport, std::vector<uint8_t>* result)
{
	size_t needed_data_size = 0;

	needed_data_size = m_packet_parser.ReadNeededDataSize(transport);
	if( needed_data_size>0 )
		return needed_data_size;

	MAssIpcCallPacket::PacketType data_type = m_packet_parser.ReadType(transport);

	switch( data_type )
	{
		case MAssIpcCallPacket::pt_call:
		{
			std::unique_ptr<std::vector<uint8_t> > in_data(new std::vector<uint8_t>);
			auto data_size = m_packet_parser.FinishReceivePacketSize();
			m_packet_parser.ReadData(transport, in_data.get(), data_size);
			InvokeLocal(std::move(in_data));
		}
		break;
		case MAssIpcCallPacket::pt_return_fail_call:
		{
			std::vector<uint8_t> fail_result;
			MAssIpcCallDataStream fail_result_str(&fail_result);
			auto data_size = m_packet_parser.FinishReceivePacketSize();
			m_packet_parser.ReadData(transport, &fail_result, data_size);
			MAssIpcCall::ErrorType error = ErrorType::unknown_error;
			std::string message;
			fail_result_str>>error;
			fail_result_str>>message;

			if( error == ErrorType::no_matching_call_name_parameters )
				error = ErrorType::respond_no_matching_call_name_parameters;
			else if( error == ErrorType::no_matching_call_return_type )
				error = ErrorType::respond_no_matching_call_return_type;
			else
				error = ErrorType::unknown_error;

			if( bool(m_OnInvalidRemoteCall) )
				m_OnInvalidRemoteCall(error, message);
		}
		break;
		case MAssIpcCallPacket::pt_enumerate_return:
		case MAssIpcCallPacket::pt_return:
		{
			mass_return_x_if_equal(result, nullptr, 0);
			auto data_size = m_packet_parser.FinishReceivePacketSize();
			m_packet_parser.ReadData(transport, result, data_size);
		}
		break;
		case MAssIpcCallPacket::pt_enumerate:
		{
			auto data_size = m_packet_parser.FinishReceivePacketSize();
			mass_return_x_if_not_equal(data_size, 0, 0);// invalid packet

			std::vector<uint8_t> result_now;
			MAssIpcCall_EnumerateData res = m_proc_map.EnumerateHandlers();
			MAssIpcCallPacket::PacketHeaderAllocate(&result_now, MAssIpcCallPacket::pt_enumerate_return);
			{
				MAssIpcCallDataStream data_raw(&result_now);
				data_raw<<res;
			}
			MAssIpcCallPacket::PacketHeaderUpdateSize(&result_now);
			transport->Write(result_now.data(), result_now.size());
		}
		break;
		default:
			mass_return_x(0);// invalid packet
			break;
	}

	return 0;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateRemote() const
{
	std::vector<uint8_t> call_info_data;
	std::vector<uint8_t> result;

	MAssIpcCallPacket::PacketHeaderAllocate(&call_info_data, MAssIpcCallPacket::pt_enumerate);
	MAssIpcCallPacket::PacketHeaderUpdateSize(&call_info_data);

	InvokeRemote(call_info_data, &result);

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

void MAssIpcCall::SetErrorHandler(TErrorHandler OnInvalidRemoteCall)
{
	m_int->m_OnInvalidRemoteCall = OnInvalidRemoteCall;
}

//-------------------------------------------------------

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall::ErrorType& v)
{
	stream<<std::underlying_type<MAssIpcCall::ErrorType>::type(v);
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall::ErrorType& v)
{
	return stream >> reinterpret_cast<std::underlying_type<MAssIpcCall::ErrorType>::type&>(v);
}
