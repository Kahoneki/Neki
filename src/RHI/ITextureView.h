#pragma once

#include "ITexture.h"

#include <Core/Memory/FreeListAllocator.h>


namespace NK
{
	
	struct TextureViewDesc
	{
		TEXTURE_VIEW_TYPE type;
		TEXTURE_DIMENSION dimension;
		DATA_FORMAT format;
		//todo: add array support
		//todo: add mip support
	};


	class ITextureView
	{
	public:
		virtual ~ITextureView() = default;
		[[nodiscard]] inline ResourceIndex GetIndex() const { return m_resourceIndex; }
		[[nodiscard]] inline DATA_FORMAT GetFormat() const { return m_format; }
		[[nodiscard]] inline TEXTURE_VIEW_TYPE GetType() const { return m_type; }


	protected:
		//If creating a texture view of a shader-accessible texture view (SHADER_READ_ONLY or SHADER_READ_WRITE), pass in a free list allocator for bindless resource allocation
		//Otherwise, just set _freeListAllocator to nullptr
		explicit ITextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, FreeListAllocator* _freeListAllocator, bool _freeListAllocated)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_type(_desc.type), m_dimension(_desc.dimension), m_format(_desc.format),
		  m_resourceIndexAllocator(_freeListAllocator), m_freeListAllocated(_freeListAllocated) {}
		
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		FreeListAllocator* m_resourceIndexAllocator;
		IDevice& m_device;
		
		bool m_freeListAllocated; //True if m_resourceIndex was allocated from a free list allocator
		ResourceIndex m_resourceIndex{ FreeListAllocator::INVALID_INDEX };
		TEXTURE_VIEW_TYPE m_type;
		TEXTURE_DIMENSION m_dimension;
		DATA_FORMAT m_format;
	};
	
}