#pragma once

#include "IAllocator.h"

#include <Core/Context.h>

#include <memory>


#define NK_NEW(Type, ...)	NK::NewImpl<Type>(__FILE__, __LINE__, ##__VA_ARGS__)
#define NK_DELETE(ptr)		NK::DeleteImpl(ptr)


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
		Deleter() = default;
		
		//Allow conversions to polymorphic derived types
		template<typename U>
		explicit inline Deleter(const Deleter<U>&) noexcept requires(std::is_convertible_v<U*, T*>) {}
	};

	template<typename T>
	using UniquePtr = std::unique_ptr<T, Deleter<T>>;
	
}