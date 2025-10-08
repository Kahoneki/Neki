#pragma once
#include <memory>

#include "Core/Context.h"
#include <iostream>

#define NK_NEW(Type, ...)	NK::NewImpl<Type>(__FILE__, __LINE__, ##__VA_ARGS__)
#define NK_DELETE(ptr)		NK::DeleteImpl(ptr)
//For static objects - disables memory leak tracking, make sure you know what you're doing!
#define NK_STATIC_NEW(Type, ...)	NK::StaticNewImpl<Type>(__FILE__, __LINE__, ##__VA_ARGS__)
#define NK_STATIC_DELETE(ptr)		NK::StaticDeleteImpl(ptr)

namespace NK
{
	
	//Implementation of NK_NEW macro
	template<typename T, typename... Args>
	T* NewImpl(const char* _file, int _line, Args&&... _args)
	{
		void* mem{ Context::GetAllocator()->Allocate(sizeof(T), _file, _line, false) };
		return std::construct_at(reinterpret_cast<T*>(mem), std::forward<Args>(_args)...);
	}



	//Implementation of NK_DELETE macro
	template<typename T>
	void DeleteImpl(T* _ptr)
	{
		if (_ptr)
		{
			std::destroy_at(_ptr);
			Context::GetAllocator()->Free(_ptr, false);
		}
	}



	//Custom std::unique_ptr wrapper that calls a custom deleter for NK_DELETE()
	template<typename T>
	struct Deleter
	{
		inline void operator()(T* _ptr) const { NK_DELETE(_ptr); }
	};

	template<typename T>
	using UniquePtr = std::unique_ptr<T, Deleter<T>>;



	//Implementation of NK_STATIC_NEW macro
	template<typename T, typename... Args>
	T* StaticNewImpl(const char* _file, int _line, Args&&... _args)
	{
		void* mem{ Context::GetAllocator()->Allocate(sizeof(T), _file, _line, true) };
		return std::construct_at(reinterpret_cast<T*>(mem), std::forward<Args>(_args)...);
	}



	//Implementation of NK_STATIC_DELETE macro
	template<typename T>
	void StaticDeleteImpl(T* _ptr)
	{
		if (_ptr)
		{
			std::destroy_at(_ptr);
			IAllocator* allocator{ Context::GetAllocator() };
			allocator->Free(_ptr, true);
		}
	}



	//Custom std::unique_ptr wrapper that calls a custom deleter for NK_STATIC_DELETE()
	template<typename T>
	struct StaticDeleter
	{
		inline void operator()(T* _ptr) const { NK_STATIC_DELETE(_ptr); }
	};

	template<typename T>
	using StaticUniquePtr = std::unique_ptr<T, StaticDeleter<T>>;
	
}