#pragma once

#include "EventHandlerMap.h"


class Sut
{
private:

	template<class TStub>
	struct DeclareStaticVariable
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
		return &DeclareStaticVariable<void>::s_sut_event_map;
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
std::weak_ptr<EventHandlerMap> Sut::DeclareStaticVariable<TStub>::s_sut_event_map;
