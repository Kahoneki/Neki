#include "D3D12Texture.h"

#include <Core/Utils/EnumUtils.h>
#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


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


		//Create texture
		switch (m_dimension)
		{
		case TEXTURE_DIMENSION::DIM_1:
		{
			m_resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			m_resourceDesc.Width = m_size.x;
			m_resourceDesc.Height = 1; //Must be 1 for 1D textures

			//If it's an array, the y component of size holds the array count
			m_resourceDesc.DepthOrArraySize = m_arrayTexture ? m_size.y : 1;
			break;
		}

		case TEXTURE_DIMENSION::DIM_2:
		{
			m_resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_resourceDesc.Width = m_size.x;
			m_resourceDesc.Height = m_size.y;

			//If it's an array, the z component of size holds the array count
			m_resourceDesc.DepthOrArraySize = m_arrayTexture ? m_size.z : 1;
			break;
		}

		case TEXTURE_DIMENSION::DIM_3:
		{
			m_resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			m_resourceDesc.Width = m_size.x;
			m_resourceDesc.Height = m_size.y;
			m_resourceDesc.DepthOrArraySize = m_size.z;
			break;
		}
		}
		m_resourceDesc.Alignment = 0;
		m_resourceDesc.MipLevels = _desc.mipLevels;
		m_resourceDesc.Format = GetDXGIFormat(m_format);
		m_resourceDesc.SampleDesc.Count = GetD3D12SampleCount();
		m_resourceDesc.SampleDesc.Quality = 0;
		m_resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		m_resourceDesc.Flags = GetCreationFlags();


		D3D12_CLEAR_VALUE clearValue{};
		D3D12_CLEAR_VALUE* pClearValue = nullptr;

		if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT))
		{
			if (m_resourceDesc.Format == DXGI_FORMAT_R32_TYPELESS)
			{
				clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			}
			else if (m_resourceDesc.Format == DXGI_FORMAT_R16_TYPELESS)
			{
				clearValue.Format = DXGI_FORMAT_D16_UNORM;
			}
			else
			{
				clearValue.Format = m_resourceDesc.Format;
			}
			clearValue.DepthStencil.Depth = 1.0f;
			clearValue.DepthStencil.Stencil = 0;
			pClearValue = &clearValue;
		}
		else if (EnumUtils::Contains(m_usage, TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT))
		{
			if (m_resourceDesc.Format == DXGI_FORMAT_R32_TYPELESS)
			{
				clearValue.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (m_resourceDesc.Format == DXGI_FORMAT_R16_TYPELESS)
			{
				clearValue.Format = DXGI_FORMAT_R16_FLOAT;
			}
			else
			{
				clearValue.Format = m_resourceDesc.Format;
			}
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 0.0f;
			pClearValue = &clearValue;
		}


		D3D12MA::ALLOCATION_DESC allocDesc{};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		//todo: D3D12MA::ALLOCATION_FLAGS has some really cool stuff i wanna play around with

		HRESULT hr{ dynamic_cast<D3D12Device&>(m_device).GetD3D12MAAllocator()->CreateResource(&allocDesc, &m_resourceDesc, D3D12_RESOURCE_STATE_COMMON, pClearValue, &m_allocation, IID_PPV_ARGS(&m_texture)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "ID3D12Resource initialisation and allocation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "ID3D12Resource initialisation or allocation unsuccessful. hr = " + std::to_string(hr) + '\n');
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
		return D3D12_RESOURCE_STATE_COMMON;
	}



	UINT D3D12Texture::GetD3D12SampleCount() const
	{
		switch (m_sampleCount)
		{
		case SAMPLE_COUNT::BIT_1:	return 1U;
		case SAMPLE_COUNT::BIT_2:	return 2U;
		case SAMPLE_COUNT::BIT_4:	return 4U;
		case SAMPLE_COUNT::BIT_8:	return 8U;
		case SAMPLE_COUNT::BIT_16:	return 16U;
		case SAMPLE_COUNT::BIT_32:	return 32U;
		default:
		{
			throw std::invalid_argument("Default case reached for D3D12Texture::GetSampleCount() - sample count = " + std::to_string(std::to_underlying(m_sampleCount)));
		}
		}
	}



	D3D12Texture::~D3D12Texture()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Shutting Down D3D12Texture\n");

		if (!m_isOwned)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TEXTURE, "Texture not owned by D3D12Texture class, skipping shutdown sequence\n");
			m_texture.Detach(); //Detach from the comptr so as to not try and decrement the reference counter
		}

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



	DXGI_FORMAT D3D12Texture::GetDXGIFormat(DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::UNDEFINED:				return DXGI_FORMAT_UNKNOWN;
		case DATA_FORMAT::R16_TYPELESS:				return DXGI_FORMAT_R16_TYPELESS;
		case DATA_FORMAT::R32_TYPELESS:				return DXGI_FORMAT_R32_TYPELESS;
		case DATA_FORMAT::R8_UNORM:					return DXGI_FORMAT_R8_UNORM;
		case DATA_FORMAT::R16_UNORM:				return DXGI_FORMAT_R16_UNORM;
		case DATA_FORMAT::R8G8_UNORM:				return DXGI_FORMAT_R8G8_UNORM;
		case DATA_FORMAT::R8G8B8A8_UNORM:			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DATA_FORMAT::R8G8B8A8_SRGB:			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DATA_FORMAT::B8G8R8A8_UNORM:			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DATA_FORMAT::B8G8R8A8_SRGB:			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

		case DATA_FORMAT::R16_SFLOAT:				return DXGI_FORMAT_R16_FLOAT;
		case DATA_FORMAT::R16G16_SFLOAT:			return DXGI_FORMAT_R16G16_FLOAT;
		case DATA_FORMAT::R16G16B16A16_SFLOAT:		return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case DATA_FORMAT::R32_SFLOAT:				return DXGI_FORMAT_R32_FLOAT;
		case DATA_FORMAT::R32G32_SFLOAT:			return DXGI_FORMAT_R32G32_FLOAT;
		case DATA_FORMAT::R32G32B32_SFLOAT:			return DXGI_FORMAT_R32G32B32_FLOAT;
		case DATA_FORMAT::R32G32B32A32_SFLOAT:		return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case DATA_FORMAT::B10G11R11_UFLOAT_PACK32:	return DXGI_FORMAT_R11G11B10_FLOAT;
		case DATA_FORMAT::R10G10B10A2_UNORM:		return DXGI_FORMAT_R10G10B10A2_UNORM;

		case DATA_FORMAT::D16_UNORM:				return DXGI_FORMAT_D16_UNORM;
		case DATA_FORMAT::D32_SFLOAT:				return DXGI_FORMAT_D32_FLOAT;
		case DATA_FORMAT::D24_UNORM_S8_UINT:		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		case DATA_FORMAT::BC1_RGB_UNORM:			return DXGI_FORMAT_BC1_UNORM;
		case DATA_FORMAT::BC1_RGB_SRGB:				return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DATA_FORMAT::BC3_RGBA_UNORM:			return DXGI_FORMAT_BC3_UNORM;
		case DATA_FORMAT::BC3_RGBA_SRGB:			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DATA_FORMAT::BC4_R_UNORM:				return DXGI_FORMAT_BC4_UNORM;
		case DATA_FORMAT::BC4_R_SNORM:				return DXGI_FORMAT_BC4_SNORM;
		case DATA_FORMAT::BC5_RG_UNORM:				return DXGI_FORMAT_BC5_UNORM;
		case DATA_FORMAT::BC5_RG_SNORM:				return DXGI_FORMAT_BC5_SNORM;
		case DATA_FORMAT::BC7_RGBA_UNORM:			return DXGI_FORMAT_BC7_UNORM;
		case DATA_FORMAT::BC7_RGBA_SRGB:			return DXGI_FORMAT_BC7_UNORM_SRGB;

		case DATA_FORMAT::R8_UINT:					return DXGI_FORMAT_R8_UINT;
		case DATA_FORMAT::R16_UINT:					return DXGI_FORMAT_R16_UINT;
		case DATA_FORMAT::R32_UINT:					return DXGI_FORMAT_R32_UINT;

		default:
		{
			throw std::runtime_error("GetDXGIFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}

}