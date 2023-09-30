#include "MAssIpcCall.h"
#include "MAssIpc_Macros.h"
#include "MAssIpc_TransportProxy.h"

using namespace MAssIpcCallInternal;

MAssIpcCall::MAssIpcCall(const std::weak_ptr<MAssIpc_Transthread>& inter_thread_nullable)
	:m_int(std::make_shared<Internals>())
{
	m_int.load()->m_inter_thread_nullable = inter_thread_nullable;
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
	std::shared_ptr<Internals> t_int(m_int.load());

	t_int->m_transport_default = std::make_shared<TransportProxy>(transport);
	t_int->m_transport = t_int->m_transport_default;
}

void MAssIpcCall::SetTransport(const std::weak_ptr<MAssIpc_TransportShare>& transport)
{
	m_int.load()->m_transport_default.reset();
	m_int.load()->m_transport = transport;
}

void MAssIpcCall::AddAllHandlers(const MAssIpcCall& other)
{
	m_int.load()->m_proc_map.AddAllProcs(other.m_int.load()->m_proc_map);
}

void MAssIpcCall::ClearAllHandlers()
{
	m_int.load()->m_proc_map.ClearAllProcs();
}

void MAssIpcCall::ClearHandlersWithTag(const void* tag)
{
	m_int.load()->m_proc_map.ClearProcsWithTag(tag);
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

MAssIpcCall::Internals::AnalizeInvokeDataRes MAssIpcCall::Internals::ReportError_FindCallInfo(const DeserializedFindCallInfo& find_call_info, ErrorType error,
																							  const MAssIpcCallInternal::ProcMap::FindCallInfoRes& find_res,
																							  const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable) const
{
	AnalizeInvokeDataRes res = {find_res.call_info};
	res.on_call_count_changed = find_res.on_call_count_changed;
	res.send_return = find_call_info.send_return;

	res.error_arg = std::make_shared<ErrorJob::Arg>(ErrorJob::Arg{error});

	// message format "return_type proc_name(params_type)"
	constexpr char message_formaters[] = " ()";
	res.error_arg->message.reserve(sizeof(message_formaters)+find_call_info.return_type.Length()+find_call_info.name.Length()+find_call_info.params_type.Length());

	res.error_arg->message.append(find_call_info.return_type.C_String(), find_call_info.return_type.Length());
	res.error_arg->message +=(message_formaters[0]);
	res.error_arg->message.append(find_call_info.name.C_String(), find_call_info.name.Length());
	res.error_arg->message +=message_formaters[1];
	res.error_arg->message.append(find_call_info.params_type.C_String(), find_call_info.params_type.Length());
	res.error_arg->message += message_formaters[2];

	SendErrorMessage(inter_thread_nullable, find_res.on_invalid_remote_call, res.error_arg);
	return res;
}

void MAssIpcCall::Internals::SendErrorMessage(const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable,
											  const std::shared_ptr<const ErrorOccured>& on_invalid_remote_call, const std::shared_ptr<ErrorJob::Arg>& error_arg)
{
	if( bool(on_invalid_remote_call) && on_invalid_remote_call->IsCallable() )
	{
		if( inter_thread_nullable )
		{
			auto thread_id = on_invalid_remote_call->m_thread_id;

			std::unique_ptr<ErrorJob> count_job(std::make_unique<ErrorJob>(on_invalid_remote_call, std::move(error_arg)));
			inter_thread_nullable->CallFromThread(thread_id, std::move(count_job));
		}
		else
			ErrorJob::Invoke(on_invalid_remote_call, std::move(error_arg));
	}
}

MAssIpcCall::Internals::AnalizeInvokeDataRes MAssIpcCall::Internals::AnalizeInvokeData(const std::shared_ptr<MAssIpc_TransportShare>& transport,
																					   const std::shared_ptr<MAssIpc_Transthread>& inter_thread_nullable,
																					   MAssIpc_DataStream& call_info_data,
																					   MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const
{
	DeserializedFindCallInfo find_call_info = DeserializeNameSignature(call_info_data);

	ProcMap::FindCallInfoRes find_res = m_proc_map.FindCallInfo(find_call_info.name, find_call_info.params_type);

	if( !bool(find_res.call_info) || !bool(find_res.invoke_base) )
		return ReportError_FindCallInfo(find_call_info, ErrorType::no_matching_call_name_parameters, find_res, inter_thread_nullable);

	if( find_call_info.return_type.Length()!=0 )
	{
		const MAssIpc_RawString return_type_call(find_res.invoke_base->GetSignature_RetType());
		if( find_call_info.return_type != return_type_call )
			return ReportError_FindCallInfo(find_call_info, ErrorType::no_matching_call_return_type, find_res, inter_thread_nullable);
	}

	return {find_res.call_info, find_res.invoke_base, find_call_info.send_return, find_res.on_call_count_changed};
}

void MAssIpcCall::Internals::StoreReturnFailCall(MAssIpc_DataStream* result_str, const MAssIpcCallInternal::ErrorJob::Arg& error_arg,
												 MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id)
{
	(*result_str)<<error_arg.error;
	(*result_str)<<error_arg.message;
}

void MAssIpcCall::Internals::InvokeLocal(MAssIpc_DataStream& call_info_data, MAssIpcCallInternal::MAssIpc_PacketParser::TCallId id) const
{
	std::shared_ptr<MAssIpc_TransportShare> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);

	const std::shared_ptr<MAssIpc_Transthread> inter_thread_nullable = m_inter_thread_nullable.lock();

	AnalizeInvokeDataRes invoke = AnalizeInvokeData(transport, inter_thread_nullable, call_info_data, id);

	if(bool(invoke.call_info))
	{
		invoke.call_info->IncrementCallCount();

		if( bool(invoke.on_call_count_changed) && invoke.on_call_count_changed->IsCallable() )
		{
			if( inter_thread_nullable )
			{
				auto thread_id = invoke.on_call_count_changed->m_thread_id;

				std::unique_ptr<CountJob> count_job(std::make_unique<CountJob>(invoke.on_call_count_changed, invoke.call_info));
				inter_thread_nullable->CallFromThread(thread_id, std::move(count_job));
			}
			else
				CountJob::Invoke(invoke.on_call_count_changed, invoke.call_info);
		}
	}

	if( invoke.invoke_base )
	{
		auto respond_id = CallJob::CalcRespondId(invoke.send_return, id);
		if( inter_thread_nullable )
		{
			auto thread_id = invoke.invoke_base->m_thread_id;

			std::unique_ptr<CallJob> call_job(std::make_unique<CallJob>(transport, m_inter_thread_nullable, call_info_data, respond_id, invoke.invoke_base));
			inter_thread_nullable->CallFromThread(thread_id, std::move(call_job));
		}
		else
			CallJob::Invoke(transport, m_inter_thread_nullable, call_info_data, invoke.invoke_base, respond_id);
	}
	else
	{// fail to call
		if( bool(invoke.error_arg) )
		{
			if( invoke.send_return )
			{
				MAssIpc_DataStream measure_size;
				StoreReturnFailCall(&measure_size, *invoke.error_arg.get(), id);
				MAssIpc_DataStream result_str(CreateDataStream(m_transport, measure_size.GetWritePos(), MAssIpc_PacketParser::PacketType::pt_return_fail_call, id));
				StoreReturnFailCall(&result_str, *invoke.error_arg.get(), id);

				// send error result message always from current thread same as message processing thread
				// may be this behavior should be configured or permanently changed 
				auto data(result_str.DetachWrite());
				if( data )
					transport->Write(std::move(data));
			}
		}
		else
			mass_assert_msg("unexpected fail without error_arg");
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
	std::shared_ptr<Internals> t_int(m_int.load());
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
		auto transport = m_int.load()->m_transport.lock();
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
			std::shared_ptr<ErrorJob::Arg> error_arg{std::make_shared<ErrorJob::Arg>(ErrorJob::Arg{ErrorType::unknown_error})};
			buffer.data>>error_arg->error;
			buffer.data>>error_arg->message;

			if( error_arg->error == ErrorType::no_matching_call_name_parameters )
				error_arg->error = ErrorType::respond_no_matching_call_name_parameters;
			else if( error_arg->error == ErrorType::no_matching_call_return_type )
				error_arg->error = ErrorType::respond_no_matching_call_return_type;
			else
				error_arg->error = ErrorType::unknown_error;

			const std::shared_ptr<MAssIpc_Transthread> inter_thread_nullable = m_inter_thread_nullable.lock();
			std::shared_ptr<const ErrorOccured> on_invalid_remote_call = m_proc_map.GetErrorHandler();
			SendErrorMessage(inter_thread_nullable, on_invalid_remote_call, error_arg);
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

	MAssIpc_DataStream call_info_data_stream = CreateDataStream(m_int.load()->m_transport, 0, MAssIpc_PacketParser::PacketType::pt_enumerate, new_id);
	InvokeRemote(call_info_data_stream, &result, new_id, false);
	result>>res;

	return res;
}

MAssIpcCall_EnumerateData MAssIpcCall::EnumerateLocal() const
{
	return m_int.load()->m_proc_map.EnumerateHandlers();
}

MAssIpcCallInternal::MAssIpc_PacketParser::TCallId MAssIpcCall::NewCallId() const
{
	std::shared_ptr<Internals> t_int(m_int.load());

	MAssIpc_ThreadSafe::unique_lock<MAssIpc_ThreadSafe::mutex> lock(t_int->m_pending_responses.m_lock);
	MAssIpcCallInternal::MAssIpc_PacketParser::TCallId new_id;

	do
	{
		if( t_int->m_pending_responses.m_incremental_id==MAssIpcCallInternal::MAssIpc_PacketParser::c_invalid_id )
			t_int->m_pending_responses.m_incremental_id++;

		new_id = t_int->m_pending_responses.m_incremental_id++;
	}
	while( t_int->m_pending_responses.m_id_data_return.find(new_id) != t_int->m_pending_responses.m_id_data_return.end() );
	
	return new_id;
}

void MAssIpcCall::SetProcessIncomingCalls(bool process_incoming_calls_default)
{
	m_int.load()->m_process_incoming_calls_default = process_incoming_calls_default;
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
