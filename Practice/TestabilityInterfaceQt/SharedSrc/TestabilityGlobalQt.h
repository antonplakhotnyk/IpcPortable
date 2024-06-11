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
	using Sut::Addr;

private:

	template<class TSpecificSut>
	struct DeleterStorageQt: QObject
	{
		DeleterStorageQt(QObject* parent, std::shared_ptr<DeleterStorage<TSpecificSut>> sut_inst)
			:QObject(parent)
			, sut_inst(sut_inst)
		{
		}

		std::shared_ptr<DeleterStorage<TSpecificSut>> sut_inst;
	};

	template<class TStub>
	struct DeclareStaticIpc
	{
		static MAssIpcCall s_ipc_global;
	};

public:

	static Addr GetArgServerAddr(const QStringList& args);


	template<class TSpecificSut>
	static void InitClient(const QStringList& args, QObject* sut_deleter_parent = QCoreApplication::instance())
	{
		if( !DeclareStaticInst<TSpecificSut>::s_sut_inst.lock() )
			if( Addr addr = GetArgServerAddr(args) )
			{
				sut_deleter_parent = InitCheckDeleterParent(sut_deleter_parent);
				if( sut_deleter_parent )
					new DeleterStorageQt<TSpecificSut>(sut_deleter_parent, Sut::InitClient<TSpecificSut>(addr));
			}
	}

	static class MAssIpcCall& Ipc()
	{
		return DeclareStaticIpc<void>::s_ipc_global;
	}

private:

	static QObject* InitCheckDeleterParent(QObject* sut_deleter_parent);

	static QString GetArgParam_String(const QStringList& args, const char* arg);
};

template<class TStub>
MAssIpcCall TestabilityGlobalQt::DeclareStaticIpc<TStub>::s_ipc_global({});
