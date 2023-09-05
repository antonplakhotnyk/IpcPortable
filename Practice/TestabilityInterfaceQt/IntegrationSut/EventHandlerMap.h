#pragma once

#include <functional>
#include <typeindex>
#include <string>
#include <map>
#include <memory>
#include <mutex>

class EventHandlerMap
{
private:

	template<typename T>
	struct GetFunctionSignature;

	// Partial specializations for function, member function and lambda/function objects
	template<typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(Args...)>
	{
		using TFuncPtr = Ret(*)(Args...);
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



	class CallerBase
	{
	public:
		constexpr CallerBase(const void* tag) :tag(tag) {};
		virtual ~CallerBase() = default;

		const void* const tag;
	};

	template<class... Args>
	class TCaller: public CallerBase
	{
	public:

		TCaller(std::function<void(Args...)> invoke, const void* tag):CallerBase{tag}, invoke(std::move(invoke))
		{
		}

		const std::function<void(Args...)> invoke;
	};


	template<class TRet, class TCls, class... Args>
	static auto GetArgsTuple_helper(TRet(TCls::* proc)(Args...), int)->std::tuple<Args...>;

	template<class TRet, class... Args>
	static auto GetArgsTuple_helper(TRet(*proc)(Args...), int)->std::tuple<Args...>;

	template<class TCall>
	static void GetArgsTuple_helper(TCall, ...)
	{
	}

	template<class TCall>
	struct TupleArgsFunction
	{
		using type = decltype(GetArgsTuple_helper(TCall(nullptr), 0));
	};


	template<class TypeCheckParams, class... Args>
	struct TKeyTypeIndex
	{
		using TKeyHandler = typename TupleArgsFunction<typename GetFunctionSignature<TypeCheckParams>::TFuncPtr>::type;
		using TKeyCall = typename std::tuple<Args...>;
		static_assert(std::is_same<TKeyHandler, TKeyCall>::value, "TypeCheckParams must be callable or function pointer with exactly same Args");

		static std::type_index HandlerTypeIndex()
		{
			return std::type_index{typeid(TKeyHandler)};
		}
	};

	template<class TypeCheckParams, class... Args>
	struct AddHandlerInfo
	{
		using TypeCheckFunction = typename std::function<void(Args...)>;
		using KeyTypeIndex = TKeyTypeIndex<TypeCheckParams, Args...>;
		using Caller = TCaller<Args...>;
	};

	template<class TypeCheckParams, class TRet, class... Args>
	static auto GetAddHandlerInfo(TRet(* proc)(Args...))->AddHandlerInfo<TypeCheckParams, Args...>;

	template<class TypeCheckParams>
	struct GetTInfo
	{
		using type = decltype(GetAddHandlerInfo<TypeCheckParams>(typename GetFunctionSignature<TypeCheckParams>::TFuncPtr(nullptr)));
	};

	struct Key
	{
		std::string name;
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
	std::map<Key, std::shared_ptr<CallerBase> >	m_name_procs;

private:

	std::shared_ptr<CallerBase> FindCaller(const Key& key)
	{
		std::unique_lock<std::mutex> lock(m_lock);

		auto it_name_params = m_name_procs.find(key);
		if( it_name_params==m_name_procs.end() )
			return nullptr;
		return it_name_params->second;
	};


public:

	template<class TypeCheckParams, class THandlerProc>
	void AddHandler(THandlerProc handler_proc, const void* tag = nullptr)
	{
		AddHandlerName<TypeCheckParams, THandlerProc>(std::string{}, std::forward<THandlerProc>(handler_proc), tag);
	}

	template<class TypeCheckParams, class THandlerProc>
	void AddHandlerName(std::string name, THandlerProc handler_proc, const void* tag = nullptr)
	{
		using TInfo = typename GetTInfo<TypeCheckParams>::type;
		const Key key{std::move(name),TInfo::KeyTypeIndex::HandlerTypeIndex()};

		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto it_name_params = m_name_procs.find(key);
			if(it_name_params != m_name_procs.end())
				m_name_procs.erase(it_name_params);
			typename TInfo::TypeCheckFunction proc(handler_proc);
			m_name_procs.emplace(std::make_pair(key, new typename TInfo::Caller(proc, tag)));
		}
	}

	template<class THandlerProc>
	void AddName(std::string name, THandlerProc handler_proc, const void* tag = nullptr)
	{
 		AddHandlerName<typename GetFunctionSignature<THandlerProc>::TFuncPtr, THandlerProc>(name, std::forward<THandlerProc>(handler_proc), tag);
	}

	template<class TypeCheckParams>
	void ClearHandler()
	{
		ClearHandlerName<TypeCheckParams>(std::string{});
	}

	template<class TypeCheckParams>
	void ClearHandlerName(std::string name)
	{
		using TInfo = typename GetTInfo<TypeCheckParams>::type;
		const Key key{std::move(name),TInfo::KeyTypeIndex::HandlerTypeIndex()};

		m_name_procs.erase(key);
	}

	void ClearHandlersWithTag(const void* tag)
	{
		std::unique_lock<std::mutex> lock(m_lock);

		for(auto it = m_name_procs.begin(); it != m_name_procs.end(); )
			if(it->second->tag == tag)
				it = m_name_procs.erase(it);
			else
				it++;
	}

	template<class TypeCheckParams, class... Args>
	void Call(Args... args)
	{
		CallName<TypeCheckParams, Args...>(std::string{}, std::forward<Args>(args)...);
	}

	template<class TypeCheckParams, class... Args>
	void CallName(std::string name, Args... args)
	{
		using TInfo = typename GetTInfo<TypeCheckParams>::type;
		const Key key{std::move(name),TInfo::KeyTypeIndex::HandlerTypeIndex()};

		if(std::shared_ptr<CallerBase> caller_base = FindCaller(key) )
			if( auto* caller = dynamic_cast<typename TInfo::Caller*>(caller_base.get()) )
				caller->invoke(args...);
	}
};
