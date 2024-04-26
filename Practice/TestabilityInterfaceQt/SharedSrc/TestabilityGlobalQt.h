#pragma once

#include "MAssIpcCall.h"

#include "MAssIpc_TransthreadTarget.h"
#include "IpcQt_Serializers.h"
#include "Sut.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QCoreApplication>

class TestabilityGlobalQt: public Sut
{
public:
	struct Addr;
private:

	template<class TSpecificSut>
	struct DeleterStorage: public QObject
	{
		DeleterStorage(QObject* parent, const TestabilityGlobalQt::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map)
			:QObject(parent)
			, specific_sut(connect_to_address, sut_event_map)
		{
		}

		TSpecificSut specific_sut;
	};

	template<class TSpecificSut>
	struct DeclareStaticVariable
	{
		static QPointer<DeleterStorage<TSpecificSut>> s_sut_inst;
	};

public:

	static constexpr uint16_t c_default_autotest_server_port = 2233;
	static constexpr char c_arg_autotest_server[] = "-autotest-server";
	static constexpr char c_arg_autotest_server_port[] = "-autotest-server-port";

	struct Addr
	{
		QString host_name;
		uint16_t target_port;

		operator bool() const
		{
			return !host_name.isEmpty();
		}
	};

	static Addr GetArgServerAddr(const QStringList& args);

	static class MAssIpcCall& Ipc();

	template<class TSpecificSut>
	static void InitClient(const QStringList& args, QObject* sut_deleter_parent = QCoreApplication::instance())
	{
		if( !DeclareStaticVariable<TSpecificSut>::s_sut_inst )
			if( Addr addr = GetArgServerAddr(args) )
			{
				sut_deleter_parent = InitCheckDeleterParent(sut_deleter_parent);
				if( sut_deleter_parent )
					DeclareStaticVariable<TSpecificSut>::s_sut_inst = new DeleterStorage<TSpecificSut>(sut_deleter_parent, addr, Sut::GetEventHandlerMap());
			}
	}

private:

	static QObject* InitCheckDeleterParent(QObject* sut_deleter_parent);

	static QString GetArgParam_String(const QStringList& args, const char* arg);
};

template<class TSpecificSut>
QPointer<TestabilityGlobalQt::DeleterStorage<TSpecificSut>> TestabilityGlobalQt::DeclareStaticVariable<TSpecificSut>::s_sut_inst;
