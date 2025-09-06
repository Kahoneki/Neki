#include "D3D12TextureView.h"

#include "D3D12Texture.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR
#endif

namespace NK
{

	D3D12TextureView::D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, FreeListAllocator* _freeListAllocator)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, _freeListAllocator)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");

		m_freeListAllocated = true;

		
		//This constructor logically requires shader-accessible view type
		//Todo: maybe look into whether it makes sense to lift this requirement but for now I can't think of a scenario where it would make sense
		if (_desc.type == TEXTURE_VIEW_TYPE::RENDER_TARGET || _desc.type == TEXTURE_VIEW_TYPE::DEPTH_STENCIL)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "This constructor is only to be used for shader-accessible view types. This requirement will maybe be lifted in a future version - if there is a valid reason for needing this constructor in this instance, please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}


		m_resourceIndex = m_resourceIndexAllocator->Allocate();
		D3D12_CPU_DESCRIPTOR_HANDLE handle{ _descriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		handle.ptr += m_resourceIndex * _descriptorSize;


		switch (_desc.type)
		{
		case TEXTURE_VIEW_TYPE::SHADER_READ_ONLY:
		{
			//Shader-resource view
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Texture2D.MipLevels = 1;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateShaderResourceView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, handle);

			break;
		}

		case TEXTURE_VIEW_TYPE::SHADER_READ_WRITE:
		{
			//Unordered-access view
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateUnorderedAccessView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), nullptr, &desc, handle);

			break;
		}
		}
		
		
		m_logger.Unindent();
	}



	D3D12TextureView::D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, std::uint32_t _resourceIndex)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, nullptr)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");

		m_freeListAllocated = false;


		//This constructor logically requires non-shader-accessible view type
		//Todo: maybe look into whether it makes sense to lift this requirement but for now I can't think of a scenario where it would make sense
		if (_desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_ONLY || _desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_WRITE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "This constructor is only to be used for non-shader-accessible view types. This requirement will maybe be lifted in a future version - if there is a valid reason for needing this constructor in this instance, please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}


		m_resourceIndex = _resourceIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE handle{ _descriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		handle.ptr += m_resourceIndex * _descriptorSize;


		switch (_desc.type)
		{
		case TEXTURE_VIEW_TYPE::RENDER_TARGET:
		{
			//Shader-resource view
			D3D12_RENDER_TARGET_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateRenderTargetView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, handle);

			break;
		}

		case TEXTURE_VIEW_TYPE::DEPTH_STENCIL:
		{
			//Unordered-access view
			D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
			desc.Format = D3D12Texture::GetDXGIFormat(m_format);
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateDepthStencilView(dynamic_cast<D3D12Texture*>(_texture)->GetResource(), &desc, handle);

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