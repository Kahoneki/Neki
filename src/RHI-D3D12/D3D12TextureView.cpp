#include "D3D12TextureView.h"

#include "D3D12Texture.h"

#include <stdexcept>


namespace NK
{

	D3D12TextureView::D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, FreeListAllocator* _freeListAllocator)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, _freeListAllocator, true)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");


		m_resourceIndex = m_resourceIndexAllocator->Allocate();
		m_handle = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_handle.ptr += m_resourceIndex * _descriptorSize;


		switch (_desc.type)
		{
		//Shader-resource view
		case TEXTURE_VIEW_TYPE::SHADER_READ_ONLY:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);

			//todo: add array support
			switch (m_dimension)
			{
			case TEXTURE_VIEW_DIMENSION::DIM_1:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				desc.Texture1D.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_2:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_3:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				desc.Texture3D.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_CUBE:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				desc.TextureCube.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_1D_ARRAY:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				desc.Texture1DArray.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MipLevels = _texture->GetMipLevels();
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_CUBE_ARRAY:
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				desc.TextureCubeArray.MipLevels = _texture->GetMipLevels();
				break;
			}
			}

			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateShaderResourceView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, m_handle);

			break;
		}

		//Unordered-access view
		case TEXTURE_VIEW_TYPE::SHADER_READ_WRITE:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);

			//todo: add array support
			switch (m_dimension)
			{
			case TEXTURE_VIEW_DIMENSION::DIM_1:
			{
				desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_2:
			{
				desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				break;
			}
			case TEXTURE_VIEW_DIMENSION::DIM_3:
			{
				desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				break;
			}
			}

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateUnorderedAccessView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), nullptr, &desc, m_handle);

			break;
		}
		
		//Depth and Depth-stencil view
		case TEXTURE_VIEW_TYPE::DEPTH:
		case TEXTURE_VIEW_TYPE::DEPTH_STENCIL:
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			if (m_parentTexture->IsArrayTexture())
			{
				desc.ViewDimension = (m_parentTexture->GetSampleCount() == SAMPLE_COUNT::BIT_1 ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY); //todo: look into possible scenarios where you would want a non-2d dsv?
				desc.Texture2DArray.FirstArraySlice = m_baseArrayLayer;
				desc.Texture2DArray.ArraySize = m_arrayLayerCount;
			}
			else
			{
				desc.ViewDimension = (m_parentTexture->GetSampleCount() == SAMPLE_COUNT::BIT_1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS); //todo: look into possible scenarios where you would want a non-2d dsv?
			}

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateDepthStencilView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, m_handle);

			break;
		}

		//Render target view
		case TEXTURE_VIEW_TYPE::RENDER_TARGET:
		{
			D3D12_RENDER_TARGET_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			if (m_parentTexture->IsArrayTexture())
			{
				desc.ViewDimension = (m_parentTexture->GetSampleCount() == SAMPLE_COUNT::BIT_1 ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY); //todo: look into possible scenarios where you would want a non-2d dsv?
				desc.Texture2DArray.FirstArraySlice = m_baseArrayLayer;
				desc.Texture2DArray.ArraySize = m_arrayLayerCount;
			}
			else
			{
				desc.ViewDimension = (m_parentTexture->GetSampleCount() == SAMPLE_COUNT::BIT_1 ? D3D12_RTV_DIMENSION_TEXTURE2D : D3D12_RTV_DIMENSION_TEXTURE2DMS); //todo: look into possible scenarios where you would want a non-2d rtv?
			}
			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateRenderTargetView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, m_handle);

			break;
		}
		}
		
		
		m_logger.Unindent();
	}



	D3D12TextureView::D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, std::uint32_t _resourceIndex)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, nullptr, false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");


		//This constructor logically requires non-shader-accessible view type
		//Todo: maybe look into whether it makes sense to lift this requirement but for now I can't think of a scenario where it would make sense
		if (_desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_ONLY || _desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_WRITE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "This constructor is only to be used for non-shader-accessible view types. This requirement will maybe be lifted in a future version - if there is a valid reason for needing this constructor in this instance, please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}


		m_resourceIndex = _resourceIndex;
		m_handle = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_handle.ptr += m_resourceIndex * _descriptorSize;


		switch (_desc.type)
		{
		case TEXTURE_VIEW_TYPE::RENDER_TARGET:
		{
			D3D12_RENDER_TARGET_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; //todo: look into possible scenarios where you would want a non-2d rtv?

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateRenderTargetView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, m_handle);

			break;
		}
		}


		m_logger.Unindent();
	}



	D3D12TextureView::~D3D12TextureView()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Shutting Down D3D12TextureView\n");
		
		if (m_freeListAllocated && m_resourceIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_resourceIndexAllocator->Free(m_resourceIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE_VIEW, "Resource Index Freed\n");
			m_resourceIndex = FreeListAllocator::INVALID_INDEX;
		}
		
		m_logger.Unindent();
	}
	
}