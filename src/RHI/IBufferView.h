#pragma once

#include "IDevice.h"


namespace NK
{

	struct BufferViewDesc
	{
		BUFFER_VIEW_TYPE type;
		std::uint64_t offset; //Offset from start of IBuffer in bytes (must be aligned to hardware specific requirements)
		std::uint64_t size; //Size of view in bytes
	};


	class IBufferView
	{
	public:
		virtual ~IBufferView() = default;

		[[nodiscard]] inline ResourceIndex GetIndex() const { return m_resourceIndex; }


	protected:
		explicit IBufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_type(_desc.type), m_offset(_desc.offset), m_size(_desc.size) {}
		
		
		//Dependency injections
		ILogger& m_logger;
		FreeListAllocator& m_allocator;
		IDevice& m_device;
		
		ResourceIndex m_resourceIndex{ FreeListAllocator::INVALID_INDEX };
		BUFFER_VIEW_TYPE m_type;
		std::uint64_t m_offset;
		std::uint64_t m_size;
	};
	
}