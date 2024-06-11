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

//-------------------------------------------------------

	template<class TData>
	class CallbackList
	{
		struct SharedLockGuard
		{
			using LockGuard = std::unique_lock<std::recursive_mutex>;

			SharedLockGuard(const std::shared_ptr<std::recursive_mutex>& lock)
			{
				if( lock )
				{
					std::unique_lock<std::recursive_mutex> guard(*lock);
					m_guard.swap(guard);
					m_shared_lock = lock;
				}
			}

			bool IsLocked() const
			{
				return m_guard.owns_lock();
			}

		private:

			std::shared_ptr<std::recursive_mutex> m_shared_lock;
			std::unique_lock<std::recursive_mutex> m_guard;
		};


		template<class...>
		using void_t = void;

		template<typename, typename = void_t<>>
		struct is_available_lock: std::false_type
		{
		};

		template<typename T>
		struct is_available_lock<T, void_t<decltype(std::declval<T>().lock())>>: std::true_type
		{
		};

		template<class Ptr>
		static auto GetPtrImpl(const Ptr& ptr, std::true_type) -> decltype(ptr.lock())
		{
			return ptr.lock();
		}

		template<class Ptr>
		static Ptr GetPtrImpl(const Ptr& ptr, std::false_type)
		{
			return ptr;
		}

		template<class Ptr>
		static auto GetPtr(const Ptr& ptr) -> decltype(GetPtrImpl(ptr, is_available_lock<Ptr>{}))
		{
			return GetPtrImpl(ptr, is_available_lock<Ptr>{});
		}

	public:

		static std::shared_ptr<std::recursive_mutex> Create_SharedLock()
		{
			return std::make_shared<std::recursive_mutex>();
		}

		CallbackList()
		{
			static_assert(std::is_convertible<typename Element::Delete_Unlocked, decltype(GetPtr(std::declval<const TData&>())->RemoveFromList_Locked())>::value
						  || (sizeof(Element)<0) || (sizeof(TData)<0), "CallbackList<TData>::Element must be a base class of TData example: struct Example: CallbackList<Example>::Element");

			m_base.m_cb = std::make_shared<CtrlBlock>(m_base.m_shared_lock);
		}

		~CallbackList()
		{
			for( std::shared_ptr<CtrlBlock> cb = this->Front(); bool(cb); cb = this->GetNext(cb) )
			{
				if( auto el = GetPtr(this->GetElement(cb)) )
					el->RemoveFromList();
			}
		}



		class CtrlBlock
		{
		public:


			CtrlBlock(const std::shared_ptr<std::recursive_mutex>& lock)
				:shared_lock(lock)
			{
			}

			~CtrlBlock()
			{
				SharedLockGuard lock(this->shared_lock);
				RemoveFromList_Locked();
			}

			bool IsInList() const
			{
				SharedLockGuard lock(this->shared_lock);
				return IsInList_Locked();
			}


		private:


			friend class CallbackList;


			const std::shared_ptr<std::recursive_mutex> shared_lock;
			TData element = {};
			std::weak_ptr<CtrlBlock> curr;

			CtrlBlock* next = this;
			CtrlBlock* prev = this;


			static CtrlBlock* PrepareAdd_Locked(const TData& add_element)
			{
				auto add_element_ptr = GetPtr(add_element);
				CtrlBlock* element_add_cb = add_element_ptr->m_cb.get();

				element_add_cb->curr = add_element_ptr->m_cb;
				return element_add_cb;
			}

			void AddAfter_Locked(const TData& element)
			{
				if( CtrlBlock* element_add_cb = PrepareAdd_Locked(element) )
				{
					element_add_cb->prev = this;
					element_add_cb->next = this->next;
					this->next->prev = element_add_cb;
					this->next = element_add_cb;
				}
			}

			void AddBefore_Locked(const TData& element)
			{
				if( CtrlBlock* element_add_cb = PrepareAdd_Locked(element) )
				{
					element_add_cb->next = this;
					element_add_cb->prev = this->prev;
					this->prev->next = element_add_cb;
					this->prev = element_add_cb;
				}
			}

			void RemoveFromList_Locked()
			{
				if( !IsInList_Locked() )
					return;

				this->prev->next = this->next;
				this->next->prev = this->prev;

				this->next = this;
				this->prev = this;
			}

			bool IsInList_Locked() const
			{
				return this->next != this;
			}

			auto GetElement_Locked() const ->decltype(this->element)
			{
				return this->element;
			}

			auto ReleaseElement_Locked() -> decltype(this->element)
			{
				return std::move(this->element);
			}
		};

		class Element
		{
		public:

			Element(const std::shared_ptr<std::recursive_mutex>& lock)
				: m_shared_lock(lock)
			{
			}

			~Element()
			{
				RemoveFromList();
			}

			void RemoveFromList()
			{
				Delete_Unlocked delete_unlocked;
				SharedLockGuard lock(m_shared_lock);
				delete_unlocked = RemoveFromList_Locked();
			}


		protected:

			struct Delete_Unlocked
			{
				decltype(CtrlBlock::element) el{};
				std::shared_ptr<CtrlBlock> cb;
			};

			Delete_Unlocked RemoveFromList_Locked()
			{
				Delete_Unlocked delete_unlocked;
				std::swap(m_cb, delete_unlocked.cb);
				if( delete_unlocked.cb )
					delete_unlocked.el = delete_unlocked.cb->ReleaseElement_Locked();
				return std::move(delete_unlocked);// destructors must be called outside lock
			}


			friend class CallbackList;

			std::shared_ptr<CtrlBlock> m_cb;
			const std::shared_ptr<std::recursive_mutex> m_shared_lock;
		};


		const std::shared_ptr<std::recursive_mutex>& GetSharedLock() const
		{
			return m_base.m_shared_lock;
		}

		auto GetElement(std::shared_ptr<CtrlBlock> current) const ->decltype(GetPtr(current->GetElement_Locked()))
		{
			const SharedLockGuard& lock = LockSameCheck(current);
			if( !lock.IsLocked() )
				return decltype(GetPtr(current->GetElement_Locked())){};
			return GetPtr(current->GetElement_Locked());
		}


		std::shared_ptr<CtrlBlock> GetNext(const std::shared_ptr<CtrlBlock>& current) const
		{
			const SharedLockGuard& lock = LockSameCheck(current);
			if( !lock.IsLocked() )
				return std::shared_ptr<CtrlBlock>{};

			if( current->next == m_base.m_cb.get() )
				return nullptr;

			std::shared_ptr<CtrlBlock> cb = current->next->curr.lock();
			return cb;
		}

		std::shared_ptr<CtrlBlock> Front() const
		{
			SharedLockGuard lock(m_base.m_shared_lock);
			if( !lock.IsLocked() )
				return std::shared_ptr<CtrlBlock>{};

			if( !m_base.m_cb->IsInList_Locked() )
				return {};
			std::shared_ptr<CtrlBlock> cb = m_base.m_cb->next->curr.lock();
			return cb;
		}

		inline void PushBack(const TData& element)
		{
			typename Element::Delete_Unlocked delete_unlocked;
			const SharedLockGuard& lock = LockAddPrepare(element, &delete_unlocked);
			if( !lock.IsLocked() )
				return;

			m_base.m_cb->AddBefore_Locked(element);
		}

		inline void PushFront(const TData& element)
		{
			typename Element::Delete_Unlocked delete_unlocked;
			const SharedLockGuard& lock = LockAddPrepare(element, &delete_unlocked);
			if( !lock.IsLocked() )
				return;

			m_base.m_cb->AddAfter_Locked(element);
		}

	private:

		SharedLockGuard LockSameCheck(const std::shared_ptr<CtrlBlock>& other_cb) const
		{
			if( other_cb->shared_lock == m_base.m_shared_lock )
				return SharedLockGuard(m_base.m_shared_lock);
			return SharedLockGuard({});
		}

		SharedLockGuard LockAddPrepare(const TData& add_element, typename Element::Delete_Unlocked* delete_unlocked) const
		{
			auto add_element_ptr = GetPtr(add_element);
			if( !add_element_ptr )
				return SharedLockGuard({});

			SharedLockGuard lock(m_base.m_shared_lock);
			if( add_element_ptr->m_cb && add_element_ptr->m_cb->IsInList_Locked() )
				*delete_unlocked = add_element_ptr->RemoveFromList_Locked();

			if( !add_element_ptr->m_cb )
				add_element_ptr->m_cb = std::make_shared<CtrlBlock>(m_base.m_shared_lock);

			add_element_ptr->m_cb->element = add_element;
			add_element_ptr->m_cb->curr = add_element_ptr->m_cb;

			return lock;
		}

	private:

		CallbackList& operator=(const CallbackList&) = delete;

		Element m_base{Create_SharedLock()};
	};

//-------------------------------------------------------

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

	template<typename Ret, typename... Args>
	struct GetFunctionSignature<Ret(*const)(Args...)>: public GetFunctionSignature<Ret(Args...)>
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
//-------------------------------------------------------

	template<class Ret, class... Args>
	static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(Ret(*a1)(Args...), Ret(*a2)(Args...))
	{
		return true;
	};

	template<class A1, class A2>
	static constexpr bool Check_is_signame_and_handler_describe_same_call_signatures(A1, A2, ...)
	{
		static_assert(std::is_same<A1, A2>::value, "Handler function signature mismatch A1 and A2 must be same types");
		return false;
	};

	class CallerBase: public CallbackList<std::shared_ptr<CallerBase>>::Element
	{
	public:
		CallerBase(const std::shared_ptr<std::recursive_mutex>& lock, const void* tag, bool skip_type_handler) :CallbackList<std::shared_ptr<CallerBase>>::Element(lock), tag(tag), skip_type_handler(skip_type_handler){};
		virtual ~CallerBase() = default;

		const void* const tag;
		const bool skip_type_handler;
	};

	template<class... Args>
	class CallerArgs: public CallerBase
	{
	public:

		CallerArgs(const std::shared_ptr<std::recursive_mutex>& lock, std::function<void(Args...)> invoke, const void* tag, bool skip_type_handler):CallerBase{lock, tag, skip_type_handler}, invoke(std::move(invoke))
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

	struct Callers
	{
		CallbackList<std::shared_ptr<CallerBase>> callers;
	};


	mutable std::mutex	m_lock;
	std::map<Key, std::shared_ptr<Callers> > m_name_procs;
	std::map<std::type_index, std::shared_ptr<Callers> > m_type_procs;

private:

	template<class TMap>
	std::shared_ptr<Callers> FindCaller_Locked(const TMap& procs_map, const typename TMap::key_type& key) const
	{
		auto it_name_params = procs_map.find(key);
		if( it_name_params==procs_map.end() )
			return {};
		return it_name_params->second;
	}

	template<typename TInfo, class THandlerProc, typename TMap, typename TKey>
	void AddHandlerProc(TMap* procs_map, const TKey& key, THandlerProc&& handler_proc, const void* tag, bool skip_type_handler)
	{
		std::unique_lock<std::mutex> lock(m_lock);

		typename TInfo::TypeCheckFunction proc(std::forward<THandlerProc>(handler_proc));
		std::shared_ptr<Callers> callers;

		auto it_name_params = procs_map->find(key);
		if( it_name_params != procs_map->end() )
			callers = it_name_params->second;
		else
		{
			callers = std::make_shared<Callers>();
			procs_map->emplace(std::make_pair(key, callers));
		}
		callers->callers.PushBack(std::make_shared<typename TInfo::Caller>(callers->callers.GetSharedLock(), proc, tag, skip_type_handler));
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
		AddHandlerProc<TInfo>(&m_name_procs, key, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
	}

	template<class TypeCheckParams, class THandlerProc>
	void AddHandlerType(THandlerProc handler_proc, const void* tag = nullptr, bool skip_type_handler = false)
	{
		using TInfo = GetInfo<Info_CallerNameArgs,TypeCheckParams>;
		static_assert(Check_is_signame_and_handler_describe_same_call_signatures(typename GetFunctionSignature<THandlerProc>::FuncPtr{}, typename GetFunctionSignature< decltype(&decltype(TInfo::Caller::invoke)::operator()) >::FuncPtr{}), "TypeCheckParams and THandlerProc signatures not match");
		auto key{TInfo::HandlerTypeIndex()};
		AddHandlerProc<TInfo>(&m_type_procs, key, std::forward<THandlerProc>(handler_proc), tag, skip_type_handler);
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
	void RemoveTagFromMap_Locked(std::map<TKey, std::shared_ptr<Callers>>& map_procs, const void* tag)
	{
		for( auto it = map_procs.begin(); it != map_procs.end(); )
		{
			auto& callers = it->second->callers;
			for( std::shared_ptr<CallbackList<std::shared_ptr<CallerBase>>::CtrlBlock> el = callers.Front(); bool(el); el = callers.GetNext(el) )
				if( std::shared_ptr<CallerBase> element = callers.GetElement(el) )
					if( element->tag == tag )
						element->RemoveFromList();
			if( !callers.Front() )
				it = map_procs.erase(it);
			else
				++it;
		}
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
		const Key key{std::move(name), TInfo::HandlerTypeIndex()};

		using TInfoName = GetInfo<Info_CallerNameArgs, TypeCheckParams>;
		const auto& key_name{TInfoName::HandlerTypeIndex()};

		std::unique_lock<std::mutex> lock(m_lock);
		std::shared_ptr<Callers> caller_base = FindCaller_Locked(m_name_procs, key);
		std::shared_ptr<Callers> caller_base_name = FindCaller_Locked(m_type_procs, key_name);
		lock.unlock();

		bool call_type_handler = !bool(caller_base);
		if( caller_base )
			for( std::shared_ptr<CallbackList<std::shared_ptr<CallerBase>>::CtrlBlock> el = caller_base->callers.Front(); bool(el); el = caller_base->callers.GetNext(el) )
				if( std::shared_ptr<CallerBase> element = caller_base->callers.GetElement(el) )
					if( auto* caller = dynamic_cast<typename TInfo::Caller*>(element.get()) )
					{
						caller->invoke(args...);
						call_type_handler |= !caller->skip_type_handler;
					}

		if( call_type_handler && bool(caller_base_name) )
			for( std::shared_ptr<CallbackList<std::shared_ptr<CallerBase>>::CtrlBlock> el = caller_base_name->callers.Front(); bool(el); el = caller_base_name->callers.GetNext(el) )
				if( std::shared_ptr<CallerBase> element = caller_base_name->callers.GetElement(el) )
					if( auto* caller = dynamic_cast<typename TInfoName::Caller*>(element.get()) )
						caller->invoke(key.name, args...);
	}

};
