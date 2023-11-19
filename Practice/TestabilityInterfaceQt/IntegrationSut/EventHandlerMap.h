#pragma once

#include <functional>
#include <typeindex>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <array>

class EventHandlerMap
{
	using event_name_t = std::string;
public:
	using event_name_arg_t = const std::string&;

private:

	template<typename T>
	struct GetFunctionSignature;

	// Partial specializations for function, member function and lambda/function objects
	template<typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(Args...)>
	{
		using FuncPtr = Ret(*)(Args...);
	};

	template<typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(*)(Args...)>: public GetFunctionSignature<Ret(Args...)>
	{
	};

	template<typename Class, typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(Class::*)(Args...)>: public GetFunctionSignature<Ret(Args...)>
	{
	};

	template<typename Class, typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(Class::*)(Args...) const>: public GetFunctionSignature<Ret(Args...)>
	{
	};

	template<typename Callable>
	struct GetFunctionSignature: public GetFunctionSignature<decltype(&Callable::operator())>
	{
	};

	template<class Ret, class... Args>
	static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(Ret(*a1)(Args...), Ret(*a2)(Args...))
	{
		return true;
	};

	template<class A1, class A2>
	static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(A1, A2, ...)
	{
		static_assert(std::is_same<A1, A2>::value, "A1 and A2 must be same types");
		return false;
	};

	class CallerBase
	{
	public:
		constexpr CallerBase(const void* tag, bool skip_type_handler) :tag(tag), skip_type_handler(skip_type_handler){};
		virtual ~CallerBase() = default;

		const void* const tag;
		const bool skip_type_handler;
	};

	template<class... Args>
	class CallerArgs: public CallerBase
	{
	public:

		CallerArgs(std::function<void(Args...)> invoke, const void* tag, bool skip_type_handler):CallerBase{tag, skip_type_handler}, invoke(std::move(invoke))
		{
		}

		const std::function<void(Args...)> invoke;
	};


	template<class... Args>
	struct Info_CallerArgs
	{
		using TypeCheckFunction = typename std::function<void(Args...)>;
		using Caller = CallerArgs<Args...>;

		static std::type_index HandlerTypeIndex()
		{
			return std::type_index{typeid(void(*)(Args...))};
		}
	};

	template<class... Args>
	struct Info_CallerNameArgs
	{
		using TypeCheckFunction = typename std::function<void(event_name_arg_t, Args...)>;
		using Caller = CallerArgs<event_name_arg_t, Args...>;

		static std::type_index HandlerTypeIndex()
		{
			return std::type_index{typeid(void(*)(event_name_arg_t, Args...))};
		}
	};


	template<template<class...> class TCaller, class Ret, class... Args>
	static auto GetInfo_TCaller_helper(Ret(* proc)(Args...))-> TCaller<Args...>;

	template<template<class...> class TCaller, class TypeCheckParams>
	struct GetInfo: decltype(GetInfo_TCaller_helper<TCaller>(typename GetFunctionSignature<TypeCheckParams>::FuncPtr(nullptr)))
	{
	};



	struct Key
	{
		event_name_t name;
		std::type_index params_type;

		bool operator<(const Key& other) const
		{
			if( params_type == other.params_type )
				return name<other.name;
			else
				return (params_type<other.params_type);
		}
	};

	mutable std::mutex	m_lock;
	std::map<Key, std::shared_ptr<CallerBase> >				m_name_procs;
	std::map<std::type_index, std::shared_ptr<CallerBase> >	m_type_procs;

private:

	template<class TMap>
	std::shared_ptr<CallerBase> FindCaller_Locked(const TMap& procs_map, const typename TMap::key_type& key) const
	{
		auto it_name_params = procs_map.find(key);
		if( it_name_params==procs_map.end() )
			return {};
		return it_name_params->second;
	};

// 	template<typename TMap, typename TKey, typename IteratorType_AddTo>
// 	static void PushCallers(IteratorType_AddTo& it, IteratorType_AddTo it_end, const TMap& procs_map, const TKey& key)
// 	{
// 		auto it_procs = procs_map.find(key);
// 		if( it_procs != procs_map.end() && it != it_end )
// 			*(it++) = it_procs->second;
// 	}
// 	 
// 	auto FindCallers(const Key& key) const
// 	{
// 		std::array<std::shared_ptr<CallerBase>, 2> callers;
// 		auto it_callers = callers.begin();
// 		auto it_callers_end = callers.end();
// 
// 		std::unique_lock<std::mutex> lock(m_lock);
// 		PushCallers(it_callers, it_callers_end, m_name_procs, key);
// 		PushCallers(it_callers, it_callers_end, m_type_procs, key.params_type);
// 
// 		return callers;
// 	};

	template<typename TInfo, class THandlerProc, typename TMap, typename TKey>
	void AddHandlerProc(TMap& procs_map, const TKey& key, THandlerProc&& handler_proc, const void* tag, bool skip_type_handler)
	{
		std::unique_lock<std::mutex> lock(m_lock);

		auto it_name_params = procs_map.find(key);
		if(it_name_params != procs_map.end())
			procs_map.erase(it_name_params);
 		typename TInfo::TypeCheckFunction proc(std::forward<THandlerProc>(handler_proc));
 		procs_map.emplace(std::make_pair(key, new typename TInfo::Caller(proc, tag, skip_type_handler)));
	}

public:

	template<class TypeCheckParams, class THandlerProc>
	void AddHandler(THandlerProc handler_proc, const void* tag = nullptr, bool skip_type_handler = false)
	{
		AddHandlerName<TypeCheckParams, THandlerProc>(event_name_t{}, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
	}

	template<class TypeCheckParams, class THandlerProc>
	void AddHandlerName(event_name_arg_t name, THandlerProc handler_proc, const void* tag = nullptr, bool skip_type_handler = false)
	{
		using TInfo = GetInfo<Info_CallerArgs,TypeCheckParams>;
		static_assert(Check_is_signame_and_handler_describe_same_call_signatures(typename GetFunctionSignature<THandlerProc>::FuncPtr{}, typename GetFunctionSignature< decltype(&decltype(TInfo::Caller::invoke)::operator()) >::FuncPtr{}), "TypeCheckParams and THandlerProc signatures not match");

		const Key key{std::move(name),TInfo::HandlerTypeIndex()};
		AddHandlerProc<TInfo>(m_name_procs, key, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
	}

	template<class TypeCheckParams, class THandlerProc>
	void AddHandlerType(THandlerProc handler_proc, const void* tag = nullptr, bool skip_type_handler = false)
	{
		using TInfo = GetInfo<Info_CallerNameArgs,TypeCheckParams>;
		static_assert(Check_is_signame_and_handler_describe_same_call_signatures(typename GetFunctionSignature<THandlerProc>::FuncPtr{}, typename GetFunctionSignature< decltype(&decltype(TInfo::Caller::invoke)::operator()) >::FuncPtr{}), "TypeCheckParams and THandlerProc signatures not match");
		auto key{TInfo::HandlerTypeIndex()};
		AddHandlerProc<TInfo>(m_type_procs, key, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
	}


	template<class THandlerProc>
	void AddName(event_name_arg_t name, THandlerProc handler_proc, const void* tag = nullptr, bool skip_type_handler = false)
	{
 		AddHandlerName<typename GetFunctionSignature<THandlerProc>::FuncPtr, THandlerProc>(name, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
	}

	template<class TypeCheckParams>
	void ClearHandler()
	{
		ClearHandlerName<TypeCheckParams>(event_name_t{});
	}

	template<class TypeCheckParams>
	void ClearHandlerName(event_name_arg_t name)
	{
		using TInfo = GetInfo<Info_CallerArgs,TypeCheckParams>;
		const Key key{std::move(name),TInfo::HandlerTypeIndex()};

		m_name_procs.erase(key);
	}

	template<class TKey>
	void RemoveTagFromMap_Locked(std::map<TKey, std::shared_ptr<CallerBase>>& map_procs, const void* tag)
	{
		for( auto it = map_procs.begin(); it != map_procs.end(); )
			if( it->second->tag == tag )
				it = map_procs.erase(it);
			else
				it++;
	}

	void ClearHandlersWithTag(const void* tag)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		RemoveTagFromMap_Locked(m_name_procs, tag);
		RemoveTagFromMap_Locked(m_type_procs, tag);
	}

	template<class TypeCheckParams, class... Args>
	void Call(Args... args)
	{
		CallName<TypeCheckParams, Args...>(event_name_t{}, std::forward<Args>(args)...);
	}

	template<class TypeCheckParams, class... Args>
	void CallName(event_name_arg_t name, Args... args)
	{
		using TInfo = GetInfo<Info_CallerArgs, TypeCheckParams>;
		const Key key{std::move(name),TInfo::HandlerTypeIndex()};

		using TInfoName = GetInfo<Info_CallerNameArgs, TypeCheckParams>;
		const auto& key_name{TInfoName::HandlerTypeIndex()};

		std::unique_lock<std::mutex> lock(m_lock);
// 		const auto& callers = FindCallers(key);
		std::shared_ptr<CallerBase> caller_base = FindCaller_Locked(m_name_procs, key);
		std::shared_ptr<CallerBase> caller_base_name = FindCaller_Locked(m_type_procs, key_name);
		lock.unlock();

		bool call_type_handler = !bool(caller_base);
// 		for( std::shared_ptr<CallerBase> caller_base : callers )
		if( caller_base )
			if( auto* caller = dynamic_cast<typename TInfo::Caller*>(caller_base.get()) )
			{
				caller->invoke(args...);
				call_type_handler |= !caller_base->skip_type_handler;
			}

		if( call_type_handler && bool(caller_base_name) )
			if( auto* caller = dynamic_cast<typename TInfoName::Caller*>(caller_base_name.get()) )
				caller->invoke(key.name, args...);
	}

};
