#include "AutotestServer_Client.h"
#include "MAssIpc_Macros.h"


void AutotestServer_Client::SutConnection::IpcError(MAssIpcCall::ErrorType et, const std::string& message)
{
	mass_assert_msg(std::string(std::string(MAssIpcCall::ErrorTypeToStr(et)) + " " + message).c_str());
}
