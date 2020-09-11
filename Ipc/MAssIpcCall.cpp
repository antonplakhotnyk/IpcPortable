#include "MAssIpcCall.h"
#include "MAssMacros.h"
#include "MAssIpcPacketTransportDefault.h"

using namespace MAssIpcCallInternal;

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssCallThreadTransport>& inter_thread_nullable)
	:m_int(new Internals)
{
	m_int->m_inter_thread_nullable = inter_thread_nullable;
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpcCallTransport>& transport)
{
	m_int->m_transport_default.reset(new MAssIpcPacketTransportDefault(transport));
	m_int->m_transport = m_int->m_transport_default;
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpcPacketTransport>& transport)
{
	m_int->m_transport_default.reset();
	m_int->m_transport = transport;
}

void MAssIpcCall::DeserializeNameSignature(MAssIpcCallDataStream& call_info, std::string* proc_name, bool* send_return, std::string* return_type, std::string* params_type)
{
	call_info>>(*proc_name)>>(*send_return)>>(*return_type)>>(*params_type);
}

MAssIpcCall::Internals::CreateCallJobRes MAssIpcCall::Internals::CreateCallJob(const std::shared_ptr<MAssIpcPacketTransport>& transport,
																			   MAssIpcCallDataStream& call_info_data,
																			   MAssIpcCallInternal::MAssIpcPacketParser::TCallId id) const
{
	CreateCallJobRes res = {};
	std::shared_ptr<MAssIpcCallInternal::CallJob> call_job(new MAssIpcCallInternal::CallJob(transport, m_inter_thread_nullable, call_info_data, id));

	{
		std::string return_type;
		std::string params_type;
		std::string proc_name;

		DeserializeNameSignature(call_job->m_call_info_data_str, &proc_name, &call_job->m_send_return, &return_type, &params_type);
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

void MAssIpcCall::Internals::StoreReturnFailCall(MAssIpcCallDataStream* result_str, const CreateCallJobRes& call_job, 
												 MAssIpcCallInternal::MAssIpcPacketParser::TCallId id)
{
	(*result_str)<<call_job.error;
	(*result_str)<<call_job.message;
}

void MAssIpcCall::Internals::InvokeLocal(MAssIpcCallDataStream& call_info_data, MAssIpcCallInternal::MAssIpcPacketParser::TCallId id) const
{
	std::shared_ptr<MAssIpcPacketTransport> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	CreateCallJobRes call_job = CreateCallJob(transport, call_info_data, id);

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
			MAssIpcCallDataStream measure_size;
			StoreReturnFailCall(&measure_size, call_job, id);
			MAssIpcCallDataStream result_str(CreateDataStream(m_transport, measure_size.GetWritePos(), MAssIpcPacketParser::PacketType::pt_return_fail_call, id));

			StoreReturnFailCall(&result_str, call_job, id);
			auto data(result_str.DetachData());
			if( data )
				transport->Write(std::move(data));
		}
	}
}

MAssIpcCall::CallDataBuffer MAssIpcCall::ReceiveCallDataBuffer(std::unique_ptr<MAssIpcData>& packet) const
{
	CallDataBuffer res = {std::move(packet),{}};
	res.header = MAssIpcPacketParser::ReadHeader(res.data);
	return res;
}

MAssIpcCallDataStream MAssIpcCall::ProcessTransportResponse(MAssIpcCallInternal::MAssIpcPacketParser::TCallId wait_response_id,
										   bool process_incoming_calls) const
{
	const bool wait_incoming_packet = (wait_response_id!=MAssIpcPacketParser::c_invalid_id);

	while( true )
	{
		MAssIpcCall::CallDataBuffer buffer;

		{// ReadBuffer
			std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
			while( true )
			{
				auto it = m_int->m_pending_responses.m_id_data_return.find(wait_response_id);
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


		if( buffer.header.type!=MAssIpcPacketParser::PacketType::pt_undefined )
		{
			bool response_precessed = (buffer.header.id == wait_response_id) &&
				((buffer.header.type >= MAssIpcPacketParser::PacketType::pt_return) &&
				 (buffer.header.type < MAssIpcPacketParser::PacketType::pt_count_not_a_type));

			MAssIpcCallDataStream result(m_int->ProcessBuffer(std::move(buffer)));
			if( response_precessed )
				return result;// wait response always return as soon as response received
		}
		else
		{
			CallDataBuffer call_data_buffer;
			bool transport_ok = false;

			{// WaitBuffer only one thread at the time here
				auto transport = m_int->m_transport.lock();
				if( transport )
				{
					std::unique_ptr<MAssIpcData> packet;
					transport_ok = transport->Read(wait_incoming_packet, &packet);
					if( packet )
						call_data_buffer = ReceiveCallDataBuffer(packet);
				}
			}

			bool process_more_incoming_calls = false;
			bool new_return_buffer = false;

			{// WriteBuffer
				std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
				
				m_int->m_pending_responses.m_another_thread_waiting_transport = false;

				if( call_data_buffer.header.type!=MAssIpcPacketParser::PacketType::pt_undefined )
				{
					if( call_data_buffer.header.type <= MAssIpcPacketParser::PacketType::pt_call )
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

			if( !transport_ok )
				return {};
			if( !wait_incoming_packet )
				if( (!process_more_incoming_calls) && (!new_return_buffer) )
					return {};
		}

	}
}

void MAssIpcCall::ProcessTransport()
{
	ProcessTransportResponse(MAssIpcPacketParser::c_invalid_id, true);
}

void MAssIpcCall::InvokeRemote(MAssIpcCallDataStream& call_info_data, MAssIpcCallDataStream* result_buf_wait_return,
							   MAssIpcPacketParser::TCallId response_id, bool process_incoming_calls) const
{
	{
		auto transport = m_int->m_transport.lock();
		mass_return_if_equal(bool(transport), false);
		auto data(call_info_data.DetachData());
		if( data )
			transport->Write(std::move(data));
		else
			return;
	}

	if( result_buf_wait_return )
		*result_buf_wait_return = ProcessTransportResponse(response_id, process_incoming_calls);
}

MAssIpcCallDataStream MAssIpcCall::Internals::ProcessBuffer(CallDataBuffer buffer) const
{
	switch( buffer.header.type )
	{
		case MAssIpcPacketParser::PacketType::pt_call:
		{
			InvokeLocal(buffer.data, buffer.header.id);
		}
		break;
		case MAssIpcPacketParser::PacketType::pt_return_fail_call:
		{
			MAssIpcCall::ErrorType error = ErrorType::unknown_error;
			std::string message;
			buffer.data>>error;
			buffer.data>>message;

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
		case MAssIpcPacketParser::PacketType::pt_return_enumerate:
		case MAssIpcPacketParser::PacketType::pt_return:
			return std::move(buffer.data);
		break;
		case MAssIpcPacketParser::PacketType::pt_enumerate:
		{
			mass_return_x_if_not_equal(buffer.header.size, 0, {});// invalid packet
			MAssIpcCall_EnumerateData res = m_proc_map.EnumerateHandlers();

			MAssIpcCallDataStream measure_size;
			measure_size<<res;

			MAssIpcCallDataStream respond_str(CreateDataStream(m_transport, measure_size.GetWritePos(), MAssIpcPacketParser::PacketType::pt_return_enumerate, buffer.header.id));
			respond_str<<res;
			std::shared_ptr<MAssIpcPacketTransport> transport = m_transport.lock();
			mass_return_x_if_equal(bool(transport), false, {});
			auto data(respond_str.DetachData());
			if( data )
				transport->Write(std::move(data));
		}
		break;
		default:
			mass_return_x({});// invalid packet
			break;
	}

	return {};
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateRemote() const
{
	MAssIpcCall_EnumerateData res;

	MAssIpcCallDataStream result;
	auto new_id = NewCallId();

	MAssIpcCallDataStream call_info_data_stream = CreateDataStream(m_int->m_transport, 0, MAssIpcPacketParser::PacketType::pt_enumerate, new_id);
	InvokeRemote(call_info_data_stream, &result, new_id, false);
	result>>res;

	return res;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateLocal() const
{
	return m_int->m_proc_map.EnumerateHandlers();
}

void MAssIpcCall::SerializeCallSignature(MAssIpcCallDataStream& call_info, const std::string& proc_name,
									 bool send_return, const std::string& return_type,
									 const std::string& params_type)
{
	call_info<<proc_name<<send_return<<return_type<<params_type;
}

void MAssIpcCall::SetErrorHandler(TErrorHandler OnInvalidRemoteCall)
{
	m_int->m_OnInvalidRemoteCall = OnInvalidRemoteCall;
}

MAssIpcCallInternal::MAssIpcPacketParser::TCallId MAssIpcCall::NewCallId() const
{
	std::unique_lock<std::mutex> lock(m_int->m_pending_responses.m_lock);
	MAssIpcCallInternal::MAssIpcPacketParser::TCallId new_id;

	{
		if( m_int->m_pending_responses.m_incremental_id==MAssIpcCallInternal::MAssIpcPacketParser::c_invalid_id )
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
