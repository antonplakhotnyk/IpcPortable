#pragma once

#include "Sut.h"
#include "IpcQt_TransthreadCaller.h"

class SutUtils: public Sut
{
private:

	template<class TProcPtr, class THandlerClass, class THandlerThis, class TRet, class... TArgs>
	static void BindHandlerImpl(const char* handler_name, TProcPtr handler_proc, THandlerThis handler_this)
	{
		static_assert(std::is_member_function_pointer<TProcPtr>::value, "member function expected");
		mass_return_if_equal(bool(handler_this), false);

		THandlerClass* clear_handler_tag = handler_this;
		MAssIpcCall& ipc = TestabilityGlobalQt::Ipc();
		ipc.AddHandler(handler_name, std::function<TRet(typename std::remove_reference<typename std::remove_cv<TArgs>::type>::type ...)>([=](TArgs&& ... args)
		{
			if( !bool(handler_this) )
				return TRet();
			return (handler_this->*handler_proc)(std::forward<TArgs>(args)...);
		}
		), {}, MAssIpc_TransthreadTarget::CurrentThread(), clear_handler_tag);
	}

protected:

	template<class THandlerClass, class THandlerThis, class TRet, class... TArgs>
	static void BindHandlerClass(const char* handler_name, TRet(THandlerClass::* handler_proc)(TArgs... args), THandlerThis handler_this)
	{
		BindHandlerImpl<decltype(handler_proc), THandlerClass, THandlerThis, TRet, TArgs...>(handler_name, handler_proc, handler_this);
	}

	template<class THandlerClass, class THandlerThis, class TRet, class... TArgs>
	static void BindHandlerClass(const char* handler_name, TRet(THandlerClass::* handler_proc)(TArgs... args) const, THandlerThis handler_this)
	{
		BindHandlerImpl<decltype(handler_proc), THandlerClass, THandlerThis, TRet, TArgs...>(handler_name, handler_proc, handler_this);
	}

public:

	template<class Object>
	using TRegUnreg = void(Object*);


	template<class Object, class SutRegisterUnRegisterProc>
	static void AddHandlerFor_SutRegUnregObject(bool register_object, SutRegisterUnRegisterProc proc, const void* tag = nullptr)
	{
		const char* proc_name = register_object ? c_register_proc_name : c_unregister_proc_name;
		if( auto event_call = Sut::GetEventHandlerMap()->lock() )
			event_call->AddHandlerName<TRegUnreg<Object>*>(proc_name, proc, tag);
	}

	template<class Object>
	static void ClearHandlerFor_SutRegUnregObject(bool register_object)
	{
		const char* proc_name = register_object ? c_register_proc_name : c_unregister_proc_name;
		if( auto event_call = Sut::GetEventHandlerMap()->lock() )
			event_call->ClearHandlerName<TRegUnreg<Object>*>(proc_name);
	}


	template<class Class_Sut, class Class, class... Args>
	static std::shared_ptr<Class_Sut> CreateRegister_Class_Sut(Args... args)
	{
		std::shared_ptr<Class_Sut> sut = std::make_shared<Class_Sut>(std::forward<Args>(args)...);
		AddHandlerFor_SutRegUnregObject<Class>(true, [=](Class* obj){sut->RegisterObject(obj);});
		AddHandlerFor_SutRegUnregObject<Class>(false, [=](Class* obj){sut->UnregisterObject(obj);});
		return sut;
	}

};
