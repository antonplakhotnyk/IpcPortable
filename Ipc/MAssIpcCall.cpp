#include "MAssIpcCall.h"
#include "MAssIpc_Macros.h"
#include "MAssIpc_TransportProxy.h"

using namespace MAssIpcCallInternal;

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable)
	:m_int(std::make_shared<Internals>())
{
	m_int->m_inter_thread_nullable = inter_thread_nullable;
}

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssIpc_TransportCopy>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable)
	: MAssIpcCall(inter_thread_nullable)
{
	SetTransport(transport);
}

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssIpc_TransportShare>& transport, const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable)
	: MAssIpcCall(inter_thread_nullable)
{
	SetTransport(transport);
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpc_TransportCopy>& transport)
{
	m_int->m_transport_default = std::make_shared<TransportProxy>(transport);
	m_int->m_transport = m_int->m_transport_default;
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpc_TransportShare>& transport)
{
	m_int->m_transport_default.reset();
	m_int->m_transport = transport;
}

void MAssIpcCall::AddAllHandlers(const MAssIpcCall& other)
{
	m_int->m_proc_map.AddAllProcs(other.m_int->m_proc_map);
}

void MAssIpcCall::ClearAllHandlers()
{
	m_int->m_proc_map.ClearAllProcs();
}

void MAssIpcCall::ClearHandlersWithTag(const void* tag)
{
	m_int->m_proc_map.ClearProcsWithTag(tag);
}

MAssIpcCall::DeserializedFindCallInfo MAssIpcCall::DeserializeNameSignature(MAssIpc_DataStream& call_info)
{
	bool send_return = false;

	// read sequence defined by packet format
	MAssIpc_RawString name			= MAssIpc_RawString::Read(call_info);
	call_info>>send_return;
	MAssIpc_RawString return_type	= MAssIpc_RawString::Read(call_info);
	MAssIpc_RawString params_type	= MAssIpc_RawString::Read(call_info);

	return {name, send_return, return_type, params_type};
}

MAssIpcCall::Internals::AnalizeInvokeDataRes MAssIpcCall::Internals::ReportError_FindCallInfo(const DeserializedFindCallInfo& find_call_info, ErrorType error) const
{
	AnalizeInvokeDataRes res = {};
	res.send_return = find_call_info.send_return;

	// message format "return_type proc_name(params_type)"
	constexpr char message_formaters[] = " ()";
	res.message.reserve(find_call_info.return_type.Length()+find_call_info.name.Length()+find_call_info.params_type.Length()+sizeof(message_formaters));

	res.message.append(find_call_info.return_type.C_String(), find_call_info.return_type.Length());
	res.message+=(message_formaters[0]);
	res.message.append(find_call_info.name.C_String(), find_call_info.name.Length());
	res.message+=message_formaters[1];
	res.message.append(find_call_info.params_type.C_String(), find_call_info.params_type.Length());
	res.message += message_formaters[2];

	res.error = error;
	if( m_OnInvalidRemoteCall )
		m_OnInvalidRemoteCall(res.error, res.message);
	return res;
}

MAssIpcCall::Internals::AnalizeInvokeDataRes MAssIpcCall::Internals::AnalizeInvokeData(const std::shared_ptr<MAssIpc_TransportShare>& transport,
																			   MAssIpc_DataStream& call_info_data,
																			   MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const
{
	DeserializedFindCallInfo find_call_info = DeserializeNameSignature(call_info_data);

	std::shared_ptr<MAssIpcCallInternal::CallInfoImpl> call_info = m_proc_map.FindCallInfo(find_call_info.name, find_call_info.params_type);
	if( !bool(call_info) )
		return ReportError_FindCallInfo(find_call_info, ErrorType::no_matching_call_name_parameters);

	if( find_call_info.return_type.Length()!=0 )
	{
		const MAssIpc_RawString return_type_call(call_info->GetSignature_RetType());
		if( find_call_info.return_type != return_type_call )
			return ReportError_FindCallInfo(find_call_info, ErrorType::no_matching_call_return_type);
	}

	return {call_info, find_call_info.send_return};
}

void MAssIpcCall::Internals::StoreReturnFailCall(MAssIpc_DataStream* result_str, const AnalizeInvokeDataRes& call_job, 
												 MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id)
{
	(*result_str)<<call_job.error;
	(*result_str)<<call_job.message;
}

void MAssIpcCall::Internals::InvokeLocal(MAssIpc_DataStream& call_info_data, MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const
{
	std::shared_ptr<MAssIpc_TransportShare> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	AnalizeInvokeDataRes invoke = AnalizeInvokeData(transport, call_info_data, id);

	if( invoke.call_info )
	{
		invoke.call_info->IncrementCallCount();
		auto inter_thread_nullable = m_inter_thread_nullable.lock();
		auto respond_id = CallJob::CalcRespondId(invoke.send_return, id);
		if( inter_thread_nullable )
		{
			auto thread_id = invoke.call_info->m_thread_id;

			std::unique_ptr<CallJob> call_job(std::make_unique<CallJob>(transport, m_inter_thread_nullable, m_OnCallCountChanged, call_info_data, respond_id, invoke.call_info));
			inter_thread_nullable->CallFromThread(thread_id, std::move(call_job));
		}
		else
			CallJob::Invoke(transport, m_inter_thread_nullable, m_OnCallCountChanged, call_info_data, invoke.call_info, respond_id);
	}
	else
	{// fail to call
		if( invoke.send_return )
		{
			MAssIpc_DataStream measure_size;
			StoreReturnFailCall(&measure_size, invoke, id);
			MAssIpc_DataStream result_str(CreateDataStream(m_transport, measure_size.GetWritePos(), MAssIpc_PacketParser::PacketType::pt_return_fail_call, id));
			StoreReturnFailCall(&result_str, invoke, id);

			// send error result message always from current thread same as message processing thread
			// may be this behavior should be configured or permanently changed 
			auto data(result_str.DetachWrite());
			if( data )
				transport->Write(std::move(data));
		}
	}
}

MAssIpcCall::CallDataBuffer MAssIpcCall::ReceiveCallDataBuffer(std::unique_ptr<const MAssIpc_Data>& packet) const
{
	CallDataBuffer res = {std::move(packet),{}};
	res.header = MAssIpc_PacketParser::ReadHeader(res.data);
	return res;
}

// Looks like it multi-thread write - multi-thread read consumer - producer queue with 
// optional exclusive callback which pushing producer and 
// optional wait for specific item in queue
MAssIpc_DataStream MAssIpcCall::ProcessTransportResponse(MAssIpcCallInternal::MAssIpc_PacketParser::TCallId wait_response_id,
										   bool process_incoming_calls) const
{
	std::shared_ptr<Internals> t_int(m_int);
	MAssIpc_ThreadSafe::unique_lock<LockCurrentThreadId> lock_thread_waiting_transport(t_int->m_pending_responses.m_thread_waiting_transport, MAssIpc_ThreadSafe::defer_lock_t());

	const bool wait_incoming_packet = (wait_response_id!=MAssIpc_PacketParser::c_invalid_id);

	while( true )
	{
		CallDataBuffer buffer;

		{// ReadBuffer
			MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(t_int->m_pending_responses.m_lock);

			if( t_int->m_pending_responses.m_thread_waiting_transport.IsCurrent() )
				return {};// transport read introduce reentrance (from same thread)

			while( true )
			{
				auto it = t_int->m_pending_responses.m_id_data_return.find(wait_response_id);
				if( it==t_int->m_pending_responses.m_id_data_return.end() )
				{
					if( process_incoming_calls && (!t_int->m_pending_responses.m_data_incoming_call.empty()) )
					{
						buffer = std::move(t_int->m_pending_responses.m_data_incoming_call.front());
						t_int->m_pending_responses.m_data_incoming_call.pop_front();
						break;// process packet extracted from queue
					}

					if( t_int->m_pending_responses.m_thread_waiting_transport.IsLocked() )
					{
						if( wait_incoming_packet )
							t_int->m_pending_responses.m_write_cond.wait(lock);
						else
							return {};// currently another thread inside transport read call, but must not wait
					}
					else
					{
						lock_thread_waiting_transport.lock();
						break;// invoke transport read
					}
				}
				else
				{
					buffer = std::move(it->second);
					t_int->m_pending_responses.m_id_data_return.erase(it);
					break;
				}
			}
		}


		if( buffer.header.type!=MAssIpc_PacketParser::PacketType::pt_undefined )
		{
			bool response_precessed = (buffer.header.id == wait_response_id) &&
				((buffer.header.type >= MAssIpc_PacketParser::PacketType::pt_return) &&
				 (buffer.header.type < MAssIpc_PacketParser::PacketType::pt_count_not_a_type));

			MAssIpc_DataStream result(t_int->ProcessBuffer(std::move(buffer)));
			if( response_precessed )
				return result;// wait response always return as soon as response received
		}
		else
		{
			CallDataBuffer call_data_buffer;
			bool transport_ok = false;

			{// WaitBuffer only one thread at the time here
				auto transport = t_int->m_transport.lock();
				if( transport )
				{
					std::unique_ptr<const MAssIpc_Data> packet;
					transport_ok = transport->Read(wait_incoming_packet, &packet);
					if( packet )
						call_data_buffer = ReceiveCallDataBuffer(packet);
				}
			}

			bool process_more_incoming_calls = false;
			bool new_return_buffer = false;

			{// WriteBuffer
				MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(t_int->m_pending_responses.m_lock);
				
				lock_thread_waiting_transport.unlock();

				if( call_data_buffer.header.type!=MAssIpc_PacketParser::PacketType::pt_undefined )
				{
					if( call_data_buffer.header.type <= MAssIpc_PacketParser::PacketType::pt_call )
						t_int->m_pending_responses.m_data_incoming_call.push_back(std::move(call_data_buffer));
					else
					{
						t_int->m_pending_responses.m_id_data_return.insert(std::make_pair(call_data_buffer.header.id, std::move(call_data_buffer)) );
						new_return_buffer = true;
					}
				}

				if( process_incoming_calls )
					process_more_incoming_calls = !t_int->m_pending_responses.m_data_incoming_call.empty();

				t_int->m_pending_responses.m_write_cond.notify_all();
			}

			if( !transport_ok )
				return {};
			if( !wait_incoming_packet )
				if( (!process_more_incoming_calls) && (!new_return_buffer) )
					return {};
		}
	}
}

void MAssIpcCall::LockCurrentThreadId::lock()
{
	mass_assert_if_not_equal(m_locked_id, MAssIpc_ThreadSafe::id());
	m_locked_id = MAssIpc_ThreadSafe::get_id();
	}

void MAssIpcCall::LockCurrentThreadId::unlock()
{
	m_locked_id = MAssIpc_ThreadSafe::id();
}

bool MAssIpcCall::LockCurrentThreadId::IsLocked() const
{
	return (m_locked_id != MAssIpc_ThreadSafe::id());
}

bool MAssIpcCall::LockCurrentThreadId::IsCurrent() const
{
	return (m_locked_id == MAssIpc_ThreadSafe::get_id());
}

void MAssIpcCall::ProcessTransport()
{
	ProcessTransportResponse(MAssIpc_PacketParser::c_invalid_id, true);
}

void MAssIpcCall::InvokeRemote(MAssIpc_DataStream& call_info_data, MAssIpc_DataStream* result_buf_wait_return,
							   MAssIpc_PacketParser::TCallId response_id, bool process_incoming_calls) const
{
	{
		auto transport = m_int->m_transport.lock();
		mass_return_if_equal(bool(transport), false);
		auto data(call_info_data.DetachWrite());
		if( data )
			transport->Write(std::move(data));
		else
			return;
	}

	if( result_buf_wait_return )
		*result_buf_wait_return = ProcessTransportResponse(response_id, process_incoming_calls);
}

MAssIpc_DataStream MAssIpcCall::Internals::ProcessBuffer(CallDataBuffer buffer) const
{
	switch( buffer.header.type )
	{
		case MAssIpc_PacketParser::PacketType::pt_call:
		{
			InvokeLocal(buffer.data, buffer.header.id);
		}
		break;
		case MAssIpc_PacketParser::PacketType::pt_return_fail_call:
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
		case MAssIpc_PacketParser::PacketType::pt_return_enumerate:
		case MAssIpc_PacketParser::PacketType::pt_return:
			return std::move(buffer.data);
		break;
		case MAssIpc_PacketParser::PacketType::pt_enumerate:
		{
			mass_return_x_if_not_equal(buffer.header.size, 0, {});// invalid packet
			MAssIpcCall_EnumerateData res = m_proc_map.EnumerateHandlers();

			MAssIpc_DataStream measure_size;
			measure_size<<res;

			MAssIpc_DataStream respond_str(CreateDataStream(m_transport, measure_size.GetWritePos(), MAssIpc_PacketParser::PacketType::pt_return_enumerate, buffer.header.id));
			respond_str<<res;
			std::shared_ptr<MAssIpc_TransportShare> transport = m_transport.lock();
			mass_return_x_if_equal(bool(transport), false, {});
			auto data(respond_str.DetachWrite());
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

	MAssIpc_DataStream result;
	auto new_id = NewCallId();

	MAssIpc_DataStream call_info_data_stream = CreateDataStream(m_int->m_transport, 0, MAssIpc_PacketParser::PacketType::pt_enumerate, new_id);
	InvokeRemote(call_info_data_stream, &result, new_id, false);
	result>>res;

	return res;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateLocal() const
{
	return m_int->m_proc_map.EnumerateHandlers();
}

void MAssIpcCall::SetErrorHandler(TErrorHandler OnInvalidRemoteCall)
{
	m_int->m_OnInvalidRemoteCall = OnInvalidRemoteCall;
}

TCallCountChanged MAssIpcCall::SetCallCountChanged(const TCallCountChanged& OnCallCountChanged)
{
	return m_int->SetCallCountChanged(OnCallCountChanged);
}

MAssIpcCallInternal::MAssIpc_PacketParser::TCallId MAssIpcCall::NewCallId() const
{
	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(m_int->m_pending_responses.m_lock);
	MAssIpcCallInternal::MAssIpc_PacketParser::TCallId new_id;

	{
		if( m_int->m_pending_responses.m_incremental_id==MAssIpcCallInternal::MAssIpc_PacketParser::c_invalid_id )
			m_int->m_pending_responses.m_incremental_id++;

		new_id = m_int->m_pending_responses.m_incremental_id++;
	}
	while( m_int->m_pending_responses.m_id_data_return.find(new_id) != m_int->m_pending_responses.m_id_data_return.end() );
	
	return new_id;
}

//-------------------------------------------------------

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const MAssIpcCall::ErrorType& v)
{
	stream<<std::underlying_type<MAssIpcCall::ErrorType>::type(v);
	return stream;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, MAssIpcCall::ErrorType& v)
{
	return stream >> reinterpret_cast<std::underlying_type<MAssIpcCall::ErrorType>::type&>(v);
}
