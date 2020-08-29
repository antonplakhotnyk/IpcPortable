#include "MAssIpcCall.h"
#include "MAssMacros.h"

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

MAssIpcCall::Internals::CreateCallJobRes MAssIpcCall::Internals::CreateCallJob(const std::shared_ptr<MAssIpcCallTransport>& transport, 
																			   std::unique_ptr<std::vector<uint8_t> > call_info_data,
																			   MAssIpcCallInternal::MAssIpcCallPacket::TCallId id) const
{
	CreateCallJobRes res = {};
	std::shared_ptr<MAssIpcCallInternal::CallJob> call_job(new MAssIpcCallInternal::CallJob(transport, m_inter_thread_nullable, std::move(call_info_data), id));

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

void MAssIpcCall::Internals::InvokeLocal(std::unique_ptr<std::vector<uint8_t> > call_info_data, MAssIpcCallInternal::MAssIpcCallPacket::TCallId id) const
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	CreateCallJobRes call_job = CreateCallJob(transport, std::move(call_info_data), id);

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
			MAssIpcCallPacket::PacketHeaderAllocate(&result, MAssIpcCallPacket::PacketType::pt_return_fail_call, id);
			result_str<<call_job.error;
			result_str<<call_job.message;
			MAssIpcCallPacket::PacketHeaderUpdateSize(&result);
			transport->Write(result.data(), result.size());
		}
	}
}

MAssIpcCall::CallDataBuffer MAssIpcCall::ReceiveCallDataBuffer(const std::shared_ptr<MAssIpcCallTransport>& transport) const
{
	CallDataBuffer res;
	res.header = m_int->m_packet_parser.FinishReceiveReadHeader(transport);

	res.data.reset(new std::vector<uint8_t>);
	m_int->m_packet_parser.ReadData(transport, res.data.get(), res.header.size);

	return res;
}

void MAssIpcCall::ProcessTransportResponse(const std::shared_ptr<MAssIpcCallTransport>& transport, 
										   std::vector<uint8_t>* result, MAssIpcCallInternal::MAssIpcCallPacket::TCallId response_id,
										   bool process_incoming_calls, bool wait_incoming_data) const
{
	while( true )
	{
		MAssIpcCall::CallDataBuffer buffer;

		{// ReadBuffer
			std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
			while( true )
			{
				auto it = m_int->m_pending_responses.m_id_data_return.find(response_id);
				if( it==m_int->m_pending_responses.m_id_data_return.end() )
				{
					if( process_incoming_calls && (!m_int->m_pending_responses.m_data_incoming_call.empty()) )
					{
						buffer = std::move(m_int->m_pending_responses.m_data_incoming_call.front());
						m_int->m_pending_responses.m_data_incoming_call.pop_front();
						break;
					}

					if( !m_int->m_pending_responses.m_another_thread_waiting_transport )
					{
						m_int->m_pending_responses.m_another_thread_waiting_transport = true;
						break;
					}
				}
				else
				{
					buffer = std::move(it->second);
					break;
				}

				m_int->m_pending_responses.m_write_cond.wait(lock);
			}
		}


		if( buffer.header.type!=MAssIpcCallPacket::PacketType::pt_undefined )
		{
			bool response_precessed = (buffer.header.id == response_id) && 
				((buffer.header.type >= MAssIpcCallPacket::PacketType::pt_return) &&
				 (buffer.header.type < MAssIpcCallPacket::PacketType::pt_count_not_a_type));
			m_int->ProcessBuffer(std::move(buffer), result, transport);
			if( response_precessed )
				return;// wait response always return as soon as response received
		}
		else
		{
			CallDataBuffer call_data_buffer;
			bool transport_ok = false;

			{// WaitBuffer only one thread at the time here
				size_t needed_data_size = 0;
				needed_data_size = m_int->m_packet_parser.ReadNeededDataSize(transport);
				if( needed_data_size>0 )
				{
					if( wait_incoming_data )
						transport_ok = transport->WaitRespond(needed_data_size);
				}
				else
					transport_ok = true;

				if( transport_ok )
				{
					needed_data_size = m_int->m_packet_parser.ReadNeededDataSize(transport);
					if( needed_data_size==0 )
						call_data_buffer = ReceiveCallDataBuffer(transport);
				}
			}


			bool process_more_incoming_calls = false;
			bool new_return_buffer = false;

			{// WriteBuffer
				std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
				
				m_int->m_pending_responses.m_another_thread_waiting_transport = false;

				if( call_data_buffer.header.type!=MAssIpcCallPacket::PacketType::pt_undefined )
				{
					if( call_data_buffer.header.type <= MAssIpcCallPacket::PacketType::pt_call )
						m_int->m_pending_responses.m_data_incoming_call.push_back(std::move(call_data_buffer));
					else
					{
						m_int->m_pending_responses.m_id_data_return.insert({call_data_buffer.header.id, std::move(call_data_buffer)});
						new_return_buffer = true;
					}
				}

				if( process_incoming_calls )
					process_more_incoming_calls = !m_int->m_pending_responses.m_data_incoming_call.empty();

				m_int->m_pending_responses.m_write_cond.notify_all();
			}

			if( (!transport_ok) && (!process_more_incoming_calls) && (!new_return_buffer) )
				return;
		}

	}
}

void MAssIpcCall::ProcessTransport()
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_int->m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	ProcessTransportResponse(transport, nullptr, MAssIpcCallPacket::c_invalid_id, true, false);
}

void MAssIpcCall::InvokeRemote(const std::vector<uint8_t>& call_info_data, std::vector<uint8_t>* result,
							   MAssIpcCallPacket::TCallId response_id, bool process_incoming_calls) const
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_int->m_transport.lock();
	mass_return_if_equal(bool(transport), false);
	transport->Write(call_info_data.data(), call_info_data.size());

	if( result )
	{
		ProcessTransportResponse(transport, result, response_id, process_incoming_calls, true);
	}
}

void MAssIpcCall::Internals::ProcessBuffer(CallDataBuffer buffer, std::vector<uint8_t>* result, const std::shared_ptr<MAssIpcCallTransport>& transport) const
{
	switch( buffer.header.type )
	{
		case MAssIpcCallPacket::PacketType::pt_call:
		{
			InvokeLocal(std::move(buffer.data), buffer.header.id);
		}
		break;
		case MAssIpcCallPacket::PacketType::pt_return_fail_call:
		{
			MAssIpcCallDataStream fail_result_str(buffer.data.get());
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
		case MAssIpcCallPacket::PacketType::pt_return_enumerate:
		case MAssIpcCallPacket::PacketType::pt_return:
		{
			mass_return_if_equal(result, nullptr);
			*result = *buffer.data.get();
		}
		break;
		case MAssIpcCallPacket::PacketType::pt_enumerate:
		{
			mass_return_if_not_equal(buffer.header.size, 0);// invalid packet

			std::vector<uint8_t> result_now;
			MAssIpcCall_EnumerateData res = m_proc_map.EnumerateHandlers();
			MAssIpcCallPacket::PacketHeaderAllocate(&result_now, MAssIpcCallPacket::PacketType::pt_return_enumerate, buffer.header.id);
			{
				MAssIpcCallDataStream data_raw(&result_now);
				data_raw<<res;
			}
			MAssIpcCallPacket::PacketHeaderUpdateSize(&result_now);
			transport->Write(result_now.data(), result_now.size());
		}
		break;
		default:
			mass_return();// invalid packet
			break;
	}
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateRemote() const
{
	std::vector<uint8_t> call_info_data;
	std::vector<uint8_t> result;

	auto new_id = NewCallId();
	MAssIpcCallPacket::PacketHeaderAllocate(&call_info_data, MAssIpcCallPacket::PacketType::pt_enumerate, new_id);
	MAssIpcCallPacket::PacketHeaderUpdateSize(&call_info_data);

	InvokeRemote(call_info_data, &result, new_id, false);

	MAssIpcCall_EnumerateData res;
	MAssIpcCallDataStream data_raw(&result);
	data_raw>>res;

	return res;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateLocal() const
{
	return m_int->m_proc_map.EnumerateHandlers();
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

MAssIpcCallInternal::MAssIpcCallPacket::TCallId MAssIpcCall::NewCallId() const
{
	std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
	MAssIpcCallInternal::MAssIpcCallPacket::TCallId new_id;

	{
		if( m_int->m_pending_responses.m_incremental_id==MAssIpcCallInternal::MAssIpcCallPacket::c_invalid_id )
			m_int->m_pending_responses.m_incremental_id++;

		new_id = m_int->m_pending_responses.m_incremental_id++;
	}
	while( m_int->m_pending_responses.m_id_data_return.find(new_id) != m_int->m_pending_responses.m_id_data_return.end() );
	
	return new_id;
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
