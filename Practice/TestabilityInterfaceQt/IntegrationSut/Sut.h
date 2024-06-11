#pragma once

#include "EventHandlerMap.h"


class Sut
{
private:

	template<class TStub>
	struct DeclareStaticMap
	{
		static std::weak_ptr<EventHandlerMap> s_sut_event_map;
	};

protected:

	template<class Object>
	using RegisterUnregisterObjectProc = void (*)(Object*);

	static constexpr const char* c_register_proc_name = "RegisterObject@Sut";
	static constexpr const char* c_unregister_proc_name = "UnregisterObject@Sut";

	static std::weak_ptr<EventHandlerMap>* GetEventHandlerMap()
	{
		return &DeclareStaticMap<void>::s_sut_event_map;
	}

public:

	static constexpr uint16_t c_default_autotest_server_port = 2233;
	static constexpr char c_arg_autotest_server[] = "-autotest-server";
	static constexpr char c_arg_autotest_server_port[] = "-autotest-server-port";

	struct Addr
	{
		std::string host_name;
		uint16_t target_port;

		operator bool() const
		{
			return !host_name.empty();
		}
	};

protected:

	template<class TSpecificSut>
	struct DeleterStorage
	{
		DeleterStorage(const Sut::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map)
			:specific_sut(connect_to_address, sut_event_map)
		{
		}

		TSpecificSut specific_sut;
	};

	template<class TSpecificSut>
	struct DeclareStaticInst
	{
		static std::weak_ptr<DeleterStorage<TSpecificSut>> s_sut_inst;
	};

	template<class TSpecificSut>
	static std::shared_ptr<DeleterStorage<TSpecificSut>> InitClient(const Addr& addr)
	{
		std::shared_ptr<DeleterStorage<TSpecificSut>> sut_inst(new DeleterStorage<TSpecificSut>(addr, Sut::GetEventHandlerMap()));
		DeclareStaticInst<TSpecificSut>::s_sut_inst = sut_inst;
		return sut_inst;
	}



public:


	// float a;
	// int b = 0;
	// Ipc::Event<decltype(&Cls::Proc)>(&a, &b);
	template<class TypeCheckParams, class... Args>
	static void EventProc(Args... args)
	{
		if( auto event_call = GetEventHandlerMap()->lock() )
			event_call->Call<TypeCheckParams>(std::forward<Args>(args)...);
	}

	template<class TypeCheckParams, class... Args>
	static void EventProcName(std::string name, Args... args)
	{
		if( auto event_call = GetEventHandlerMap()->lock() )
			event_call->CallName<TypeCheckParams>(std::move(name), std::forward<Args>(args)...);
	}

	template<class... Args>
	static void EventName(std::string name, Args... args)
	{
		if( auto event_call = GetEventHandlerMap()->lock() )
            event_call->CallName<void (*)(Args...)>(std::move(name), args...);
    }



	template<class Object>
	static void RegisterObject(Object* object)
	{
		if( auto event_call = GetEventHandlerMap()->lock() )
			event_call->CallName<RegisterUnregisterObjectProc<Object> >(c_register_proc_name, object);
	}

	template<class Object>
	static void UnregisterObject(Object* object)
	{
		if( auto event_call = GetEventHandlerMap()->lock() )
			event_call->CallName<RegisterUnregisterObjectProc<Object> >(c_unregister_proc_name, object);
	}



	template<class Object, class HandlerProc>
	static void AddHandler_RegisterObject(const HandlerProc& handler, const void* tag = nullptr)
	{
		if( auto event_handler = Sut::GetEventHandlerMap()->lock() )
			event_handler->AddHandlerName<RegisterUnregisterObjectProc<Object> >(Sut::c_register_proc_name, handler, tag);
	}

	template<class Object, class HandlerProc>
	static void AddHandler_UnregisterObject(const HandlerProc& handler, const void* tag = nullptr)
	{
		if( auto event_handler = Sut::GetEventHandlerMap()->lock() )
			event_handler->AddHandlerName<RegisterUnregisterObjectProc<Object> >(Sut::c_unregister_proc_name, handler, tag);
	}

	template<class Object>
	static void ClearHandler_RegisterObject()
	{
		if( auto event_call = Sut::GetEventHandlerMap()->lock() )
			event_call->ClearHandlerName<RegisterUnregisterObjectProc<Object>>(Sut::c_register_proc_name);
	}

	template<class Object>
	static void ClearHandler_UnregisterObject()
	{
		if( auto event_call = Sut::GetEventHandlerMap()->lock() )
			event_call->ClearHandlerName<RegisterUnregisterObjectProc<Object>>(Sut::c_unregister_proc_name);
	}


	void ClearHandlersWithTag(const void* tag)
	{
		if( auto event_handler = Sut::GetEventHandlerMap()->lock() )
			event_handler->ClearHandlersWithTag(tag);
	}
};

//-------------------------------------------------------
template<class TStub>
std::weak_ptr<EventHandlerMap> Sut::DeclareStaticMap<TStub>::s_sut_event_map;

template<class TSpecificSut>
std::weak_ptr<Sut::DeleterStorage<TSpecificSut>> Sut::DeclareStaticInst<TSpecificSut>::s_sut_inst;
