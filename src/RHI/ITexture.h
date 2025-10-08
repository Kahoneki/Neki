#pragma once
#include <Core/Debug/ILogger.h>
#include <glm/glm.hpp>
#include <Types/DataFormat.h>
#include <Core/Memory/IAllocator.h>

namespace NK
{
	class IDevice;
	
	enum class TEXTURE_USAGE_FLAGS : std::uint32_t
	{
		NONE                     = 1 << 0,
		TRANSFER_SRC_BIT         = 1 << 1,
		TRANSFER_DST_BIT         = 1 << 2,
		READ_ONLY                = 1 << 3,
		READ_WRITE               = 1 << 4,
		COLOUR_ATTACHMENT        = 1 << 5,
		DEPTH_STENCIL_ATTACHMENT = 1 << 6,
	};

	enum class TEXTURE_DIMENSION
	{
		DIM_1,
		DIM_2,
		DIM_3,
	};
}

//Enable bitmask operators for the TEXTURE_USAGE_FLAG_BITS type
template<>
struct enable_bitmask_operators<NK::TEXTURE_USAGE_FLAGS> : std::true_type {};

namespace NK
{

	struct TextureDesc
	{
		//Size in texels of all 3 dimensions - unused dimensions should be 1 (e.g.: for 2D textures, size.z will be ignored) - unless arrayTexture=true
		glm::ivec3 size;

		bool arrayTexture; //If true, the last populated channel will be treated as the number of array elements. If false, it will be treated as the corresponding dimension size
		TEXTURE_USAGE_FLAGS usage;
		DATA_FORMAT format;
		TEXTURE_DIMENSION dimension; //Dimensionality of the image. If arrayTexture = true, this should be the dimensionality of the composing images (i.e.: max = TEXTURE_DIMENSION::DIM_2)
	};


	class ITexture
	{
	public:
		virtual ~ITexture() = default;

		[[nodiscard]] inline glm::ivec3 GetSize() const { return m_size; }
		[[nodiscard]] inline DATA_FORMAT GetFormat() const { return m_format; }
		[[nodiscard]] inline TEXTURE_USAGE_FLAGS GetUsageFlags() const { return m_usage; }


	protected:
		explicit ITexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, bool _isOwned)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_size(_desc.size), m_arrayTexture(_desc.arrayTexture), m_usage(_desc.usage), m_format(_desc.format), m_dimension(_desc.dimension),
		  m_isOwned(_isOwned) {}



		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		glm::ivec3 m_size;
		bool m_arrayTexture;
		TEXTURE_USAGE_FLAGS m_usage;
		DATA_FORMAT m_format;
		TEXTURE_DIMENSION m_dimension;

		//True if the lifetime of the texture is to be managed by this class (i.e. if the implemented derived constructor is called)
		//False is the lifetime of the texture is to be managed elsewhere (i.e. if the implemented wrapper constructor is called)
		//See VulkanTexture / D3D12Texture for an example of the above
		//This flag exists mainly for the swapchain which in Vulkan/D3D12, owns its images
		//^trying to destroy the images yourself (e.g. in this class' destructor) results in a crash
		bool m_isOwned;
	};
}