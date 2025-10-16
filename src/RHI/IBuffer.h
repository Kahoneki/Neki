#pragma once

#include "IDevice.h"

#include <Core/Debug/ILogger.h>


namespace NK
{

	struct BufferDesc
	{
		//Size in bytes
		std::size_t size;
		MEMORY_TYPE type;
		BUFFER_USAGE_FLAGS usage;
	};


	class IBuffer
	{
	public:
		virtual ~IBuffer() = default;

		virtual void* Map() = 0;
		virtual void Unmap() = 0;

		[[nodiscard]] inline std::size_t GetSize() const { return m_size; }
		[[nodiscard]] inline MEMORY_TYPE GetMemoryType() const { return m_memType; }
		[[nodiscard]] inline BUFFER_USAGE_FLAGS GetUsageFlags() const { return m_usage; }


	protected:
		explicit IBuffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_size(_desc.size), m_memType(_desc.type), m_usage(_desc.usage) {}

		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		std::size_t m_size;
		MEMORY_TYPE m_memType;
		BUFFER_USAGE_FLAGS m_usage;
	};
	
}