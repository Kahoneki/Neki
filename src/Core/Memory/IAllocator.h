#pragma once
#include <cstdint>

namespace NK
{
	class IAllocator
	{
	public:
		virtual ~IAllocator();

		virtual void* Allocate(std::size_t _size, const char* _file, int _line) = 0;
		virtual void* Reallocate(void* _original, std::size_t _size, const char* _file, int line) = 0;
		virtual void Free(void* _ptr) = 0;
	};
}