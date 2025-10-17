#pragma once

#include "IAllocator.h"

#include <memory>


#define NK_NEW(Type, ...)	NK::NewImpl<Type>(__FILE__, __LINE__, ##__VA_ARGS__)
#define NK_DELETE(ptr)		NK::DeleteImpl(ptr)


namespace NK
{

	//Logically owned (initialised and freed) by Engine class
	//In here because of circular dependency in Engine class requiring UniquePtr
	class PrivateAllocator
	{
		friend class Engine;
		
		template<typename T, typename... Args>
		friend T* NewImpl(const char* _file, int _line, Args&&... _args);

		template<typename T>
		friend void DeleteImpl(T* _ptr);
		
		
	private:
		static IAllocator* m_allocator;
	};

	
	
	//Implementation of NK_NEW macro
	template<typename T, typename... Args>
	T* NewImpl(const char* _file, int _line, Args&&... _args)
	{
		void* mem{ PrivateAllocator::m_allocator->Allocate(sizeof(T), _file, _line, false) };
		return std::construct_at(reinterpret_cast<T*>(mem), std::forward<Args>(_args)...);
	}



	//Implementation of NK_DELETE macro
	template<typename T>
	void DeleteImpl(T* _ptr)
	{
		if (_ptr)
		{
			std::destroy_at(_ptr);
			PrivateAllocator::m_allocator->Free(_ptr, false);
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
	
}