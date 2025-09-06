#pragma once
#include "ITexture.h"

namespace NK
{
	enum class TEXTURE_VIEW_TYPE
	{
		RENDER_TARGET,
		DEPTH_STENCIL,
		
		SHADER_READ_ONLY,
		SHADER_READ_WRITE,
	};
	
	struct TextureViewDesc
	{
		TEXTURE_VIEW_TYPE type;
		TEXTURE_DIMENSION dimension;
		TEXTURE_FORMAT format;
		//todo: add array support
		//todo: add mip support
	};


	class ITextureView
	{
	public:
		//If creating a texture view of a shader-accessible texture view (SHADER_READ_ONLY or SHADER_READ_WRITE), pass in a free list allocator for bindless resource allocation
		//Otherwise, just set _freeListAllocator to nullptr
		explicit ITextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, FreeListAllocator* _freeListAllocator)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_type(_desc.type), m_dimension(_desc.dimension), m_format(_desc.format), m_resourceIndexAllocator(_freeListAllocator) {}
		virtual ~ITextureView() = default;

		[[nodiscard]] inline ResourceIndex GetIndex() const { return m_resourceIndex; }


	protected:
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		FreeListAllocator* m_resourceIndexAllocator;
		IDevice& m_device;
		
		bool m_freeListAllocated; //True if m_resourceIndex was allocated from a free list allocator
		ResourceIndex m_resourceIndex{ FreeListAllocator::INVALID_INDEX };
		TEXTURE_VIEW_TYPE m_type;
		TEXTURE_DIMENSION m_dimension;
		TEXTURE_FORMAT m_format;
	};
}