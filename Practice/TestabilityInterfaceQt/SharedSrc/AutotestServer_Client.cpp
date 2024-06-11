#include "AutotestServer_Client.h"


bool AutotestServer_Client::SetConnectionLocked(SutIndexId sut_id, std::weak_ptr<SutConnection> connection)
{
	return_x_if_equal_mass_ipc(sut_id<SutIndexId::max_count, false, false);
	m_client_connections[size_t(sut_id)] = connection;
	return true;
}

//-------------------------------------------------------

void AutotestServer_Client::SutConnection::IpcError(MAssIpcCall::ErrorType et, const std::string& message)
{
	assert_msg_mass_ipc(std::string(std::string(MAssIpcCall::ErrorTypeToStr(et)) + " " + message).c_str());
}
