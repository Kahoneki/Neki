#include "D3D12Texture.h"
#include "Core/Utils/EnumUtils.h"
#include "Core/Utils/FormatUtils.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR
#endif

namespace NK
{

	D3D12Texture::D3D12Texture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc)
	: ITexture(_logger, _allocator, _device, _desc, true)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising D3D12Texture\n");


		//m_dimension represents the dimensionality of the underlying image
		//If m_arrayTexture == true and m_dimension == TEXTURE_DIMENSION::DIM_3, this likely means there has been a misunderstanding of the parameters
		if (m_arrayTexture && m_dimension == TEXTURE_DIMENSION::DIM_3)
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "_desc.arrayTexture=true, _desc.dimension=TEXTURE_DIMENSION::DIM_3. The dimension parameter represents the dimensionality of the underlying texture and an array of 3D textures is not supported. Did you mean to set _desc.dimension=TEXTURE_DIMENSION::DIM_2?\n");
			throw std::runtime_error("");
		}


		//Define heap props
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; //Device-local
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;


		//Create texture
		D3D12_RESOURCE_DESC textureDesc{};

		switch (m_dimension)
		{
		case TEXTURE_DIMENSION::DIM_1:
		{
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			textureDesc.Width = m_size.x;
			textureDesc.Height = 1; //Must be 1 for 1D textures

			//If it's an array, the y component of size holds the array count
			textureDesc.DepthOrArraySize = m_arrayTexture ? m_size.y : 1;
			break;
		}

		case TEXTURE_DIMENSION::DIM_2:
		{
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Width = m_size.x;
			textureDesc.Height = m_size.y;

			//If it's an array, the z component of size holds the array count
			textureDesc.DepthOrArraySize = m_arrayTexture ? m_size.z : 1;
			break;
		}

		case TEXTURE_DIMENSION::DIM_3:
		{
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			textureDesc.Width = m_size.x;
			textureDesc.Height = m_size.y;
			textureDesc.DepthOrArraySize = m_size.z;
			break;
		}
		}
		textureDesc.Alignment = 0;
		textureDesc.MipLevels = 1;
		textureDesc.Format = GetDXGIFormat(m_format);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = GetCreationFlags();


		HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, GetInitialState(), nullptr, IID_PPV_ARGS(&m_texture)) };

		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "ID3D12Resource initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "ID3D12Resource initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	D3D12Texture::D3D12Texture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, ID3D12Resource* _resource)
	: ITexture(_logger, _allocator, _device, _desc, false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising D3D12Texture (wrapping existing ID3D12Resource)\n");

		m_texture = _resource;

		m_logger.Unindent();
	}



	D3D12_RESOURCE_STATES D3D12Texture::GetInitialState() const
	{
		if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT))
		{
			return D3D12_RESOURCE_STATE_COPY_DEST;
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}



	D3D12Texture::~D3D12Texture()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Shutting Down D3D12Texture\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	D3D12_RESOURCE_FLAGS D3D12Texture::GetCreationFlags() const
	{
		D3D12_RESOURCE_FLAGS d3d12Flags{ D3D12_RESOURCE_FLAG_NONE };

		if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT))
		{
			d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT))
		{
			d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::READ_WRITE))
		{
			d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		return d3d12Flags;
	}



	DXGI_FORMAT D3D12Texture::GetDXGIFormat(TEXTURE_FORMAT _format)
	{
		switch (_format)
		{
		case TEXTURE_FORMAT::R8_UNORM:					return DXGI_FORMAT_R8_UNORM;
		case TEXTURE_FORMAT::R8G8_UNORM:				return DXGI_FORMAT_R8G8_UNORM;
		case TEXTURE_FORMAT::R8G8B8A8_UNORM:			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXTURE_FORMAT::R8G8B8A8_SRGB:				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case TEXTURE_FORMAT::B8G8R8A8_UNORM:			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case TEXTURE_FORMAT::B8G8R8A8_SRGB:				return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

		case TEXTURE_FORMAT::R16_SFLOAT:				return DXGI_FORMAT_R16_FLOAT;
		case TEXTURE_FORMAT::R16G16_SFLOAT:				return DXGI_FORMAT_R16G16_FLOAT;
		case TEXTURE_FORMAT::R16G16B16A16_SFLOAT:		return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case TEXTURE_FORMAT::R32_SFLOAT:				return DXGI_FORMAT_R32_FLOAT;
		case TEXTURE_FORMAT::R32G32_SFLOAT:				return DXGI_FORMAT_R32G32_FLOAT;
		case TEXTURE_FORMAT::R32G32B32A32_SFLOAT:		return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case TEXTURE_FORMAT::B10G11R11_UFLOAT_PACK32:	return DXGI_FORMAT_R11G11B10_FLOAT;
		case TEXTURE_FORMAT::R10G10B10A2_UNORM:			return DXGI_FORMAT_R10G10B10A2_UNORM;

		case TEXTURE_FORMAT::D16_UNORM:					return DXGI_FORMAT_D16_UNORM;
		case TEXTURE_FORMAT::D32_SFLOAT:				return DXGI_FORMAT_D32_FLOAT;
		case TEXTURE_FORMAT::D24_UNORM_S8_UINT:			return DXGI_FORMAT_D24_UNORM_S8_UINT;

		case TEXTURE_FORMAT::BC1_RGB_UNORM:				return DXGI_FORMAT_BC1_UNORM;
		case TEXTURE_FORMAT::BC1_RGB_SRGB:				return DXGI_FORMAT_BC1_UNORM_SRGB;
		case TEXTURE_FORMAT::BC3_RGBA_UNORM:			return DXGI_FORMAT_BC3_UNORM;
		case TEXTURE_FORMAT::BC3_RGBA_SRGB:				return DXGI_FORMAT_BC3_UNORM_SRGB;
		case TEXTURE_FORMAT::BC4_R_UNORM:				return DXGI_FORMAT_BC4_UNORM;
		case TEXTURE_FORMAT::BC4_R_SNORM:				return DXGI_FORMAT_BC4_SNORM;
		case TEXTURE_FORMAT::BC5_RG_UNORM:				return DXGI_FORMAT_BC5_UNORM;
		case TEXTURE_FORMAT::BC5_RG_SNORM:				return DXGI_FORMAT_BC5_SNORM;
		case TEXTURE_FORMAT::BC7_RGBA_UNORM:			return DXGI_FORMAT_BC7_UNORM;
		case TEXTURE_FORMAT::BC7_RGBA_SRGB:				return DXGI_FORMAT_BC7_UNORM_SRGB;

		case TEXTURE_FORMAT::R32_UINT:					return DXGI_FORMAT_R32_UINT;

		default:
		{
			throw std::runtime_error("GetDXGIFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}
}