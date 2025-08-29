#pragma once
#include "IDevice.h"
#include "Core/Debug/ILogger.h"
#include "ResourceStates.h"

namespace NK
{
	//Bitfield of states the buffer is allowed to occupy
	//For compatibility, this must be specified at creation and cannot be changed
	enum class BUFFER_USAGE_FLAGS : std::uint32_t
	{
		NONE				= 1 << 0,
		TRANSFER_SRC_BIT	= 1 << 1,
		TRANSFER_DST_BIT	= 1 << 2,
		UNIFORM_BUFFER_BIT	= 1 << 3,
		STORAGE_BUFFER_BIT	= 1 << 4,
		VERTEX_BUFFER_BIT	= 1 << 5,
		INDEX_BUFFER_BIT	= 1 << 6,
		INDIRECT_BUFFER_BIT	= 1 << 7,
	};
}

//Enable bitmask operators for the BUFFER_USAGE_FLAG_BITS type
template<>
struct enable_bitmask_operators<NK::BUFFER_USAGE_FLAGS> : std::true_type {};

namespace NK
{
	enum class MEMORY_TYPE
	{
		HOST,
		DEVICE,
	};

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