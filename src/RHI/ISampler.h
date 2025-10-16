#pragma once

#include "IDevice.h"

#include <glm/glm.hpp>


namespace NK
{
	
	struct SamplerDesc
	{
		FILTER_MODE minFilter;
		FILTER_MODE magFilter;
		FILTER_MODE mipmapFilter;

		ADDRESS_MODE addressModeU;
		ADDRESS_MODE addressModeV;
		ADDRESS_MODE addressModeW;

		float mipLODBias;
		float maxAnisotropy{ 1.0f }; //Disabled by default
		COMPARE_OP compareOp;
		float minLOD;
		float maxLOD;
		glm::vec4 borderColour;
	};
	
	
	class ISampler
	{
	public:
		virtual ~ISampler() = default;
		
		[[nodiscard]] inline SamplerIndex GetIndex() const { return m_samplerIndex; }

	protected:
		explicit ISampler(ILogger& _logger, IAllocator& _allocator, FreeListAllocator& _freeListAllocator, IDevice& _device, const SamplerDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_freeListAllocator(_freeListAllocator), m_device(_device) {}


		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		FreeListAllocator& m_freeListAllocator;
		IDevice& m_device;

		SamplerIndex m_samplerIndex{ FreeListAllocator::INVALID_INDEX };
	};

}