#pragma once

#include <Core/Debug/ILogger.h>
#include <Core/Memory/IAllocator.h>
#include <Core/Utils/Serialisation.h>
#include <Types/NekiTypes.h>

#include <stdexcept>
#include <glm/glm.hpp>


namespace NK
{
	
	class ICommandBuffer;
	class IDevice;

	struct TextureDesc
	{
		//Size in texels of all 3 dimensions - unused dimensions should be 1 (e.g.: for 2D textures, size.z will be ignored) - unless arrayTexture=true
		glm::ivec3 size;

		bool arrayTexture; //If true, the last populated channel will be treated as the number of array elements. If false, it will be treated as the corresponding dimension size
		TEXTURE_USAGE_FLAGS usage;
		DATA_FORMAT format;
		TEXTURE_DIMENSION dimension; //Dimensionality of the image. If arrayTexture = true, this should be the dimensionality of the composing images (i.e.: max = TEXTURE_DIMENSION::DIM_2)

		bool cubemap; //If true, arrayTexture must also be true

		SAMPLE_COUNT sampleCount{ SAMPLE_COUNT::BIT_1 }; //If sampleCount > SAMPLE_COUNT::BIT_1, multisampling will be enabled
	};


	class ITexture
	{
		friend class ICommandBuffer;

	public:
		virtual ~ITexture() = default;

		[[nodiscard]] inline glm::ivec3 GetSize() const { return m_size; }
		[[nodiscard]] inline DATA_FORMAT GetFormat() const { return m_format; }
		[[nodiscard]] inline TEXTURE_USAGE_FLAGS GetUsageFlags() const { return m_usage; }
		[[nodiscard]] inline SAMPLE_COUNT GetSampleCount() const { return m_sampleCount; }
		[[nodiscard]] inline bool IsArrayTexture() const { return m_arrayTexture; }
		[[nodiscard]] inline TEXTURE_DIMENSION GetDimension() const { return m_dimension; }
		[[nodiscard]] inline RESOURCE_STATE GetState() const { return m_state; }


	protected:
		explicit ITexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, bool _isOwned)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_size(_desc.size), m_arrayTexture(_desc.arrayTexture), m_usage(_desc.usage), m_format(_desc.format), m_dimension(_desc.dimension), m_cubeMap(_desc.cubemap), m_sampleCount(_desc.sampleCount),
		  m_isOwned(_isOwned), m_state(RESOURCE_STATE::UNDEFINED)
		{
			if (m_cubeMap && !m_arrayTexture)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "Attempted to create a texture with _desc.cubeMap = true, but _desc.cubeMap = false. Neki requires that if m_cubeMap is true, m_arrayTexture must also be set to true.\n");
				throw std::runtime_error("");
			}
		}



		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		glm::ivec3 m_size;
		bool m_arrayTexture;
		TEXTURE_USAGE_FLAGS m_usage;
		DATA_FORMAT m_format;
		TEXTURE_DIMENSION m_dimension;
		bool m_cubeMap;
		SAMPLE_COUNT m_sampleCount;

		//True if the lifetime of the texture is to be managed by this class (i.e. if the implemented derived constructor is called)
		//False is the lifetime of the texture is to be managed elsewhere (i.e. if the implemented wrapper constructor is called)
		//See VulkanTexture / D3D12Texture for an example of the above
		//This flag exists mainly for the swapchain which in Vulkan/D3D12, owns its images
		//^trying to destroy the images yourself (e.g. in this class' destructor) results in a crash
		bool m_isOwned;

		RESOURCE_STATE m_state; //For enforcing strict state, updated by ICommandBuffer::TransitionBarrier()
	};
	
}