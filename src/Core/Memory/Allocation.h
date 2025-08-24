#pragma once
#include "Core/Context.h"

#define NK_NEW(Type, ...)	NK::NewImpl<Type>(__FILE__, __LINE__, ##__VA_ARGS__)
#define NK_DELETE(ptr)		NK::DeleteImpl(ptr)

namespace NK
{
	//Implementation of NK_NEW macro
	template<typename T, typename... Args>
	T* NewImpl(const char* _file, int _line, Args&&... _args)
	{
		void* mem{ Context::GetAllocator()->Allocate(sizeof(T), _file, _line) };
		return std::construct_at(reinterpret_cast<T*>(mem), std::forward<Args>(_args)...);
	}



	//Implementation of NK_DELETE macro
	template<typename T>
	void DeleteImpl(T* _ptr)
	{
		if (_ptr)
		{
			std::destroy_at(_ptr);
			Context::GetAllocator()->Free(_ptr);
		}
	}
}
