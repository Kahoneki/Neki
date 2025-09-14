#include "D3D12Swapchain.h"
#include "D3D12Device.h"
#include "D3D12Surface.h"
#include "D3D12Texture.h"
#include "D3D12TextureView.h"
#ifdef ERROR
	#undef ERROR
#endif
#include <stdexcept>


namespace NK
{

	D3D12Swapchain::D3D12Swapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc)
	: ISwapchain(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SWAPCHAIN, "Initialising D3D12Swapchain\n");

		CreateSwapchain();
		CreateRenderTargetViews();

		m_logger.Unindent();
	}



	D3D12Swapchain::~D3D12Swapchain()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SWAPCHAIN, "Shutting Down D3D12Swapchain\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	void D3D12Swapchain::CreateSwapchain()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating swapchain\n");

		const D3D12Device& device{ dynamic_cast<D3D12Device&>(m_device) };
		const D3D12Surface* surface{ dynamic_cast<D3D12Surface*>(m_surface) };


		//Describe swapchain
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
		swapchainDesc.BufferCount = m_numBuffers;
		swapchainDesc.Width = m_surface->GetSize().x;
		swapchainDesc.Height = m_surface->GetSize().y;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchainDesc.SampleDesc.Count = 1;


		//Create swapchain
		Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapchain;
		HRESULT hr{ device.GetFactory()->CreateSwapChainForHwnd(device.GetGraphicsQueue(), surface->GetSurface(), &swapchainDesc, nullptr, nullptr, &tempSwapchain) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Successfully created swapchain\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SWAPCHAIN, "Failed to create swapchain\n");
			throw std::runtime_error("");
		}

		
		//Cast to IDXGISwapChain3
		hr = tempSwapchain.As(&m_swapchain);
		if (FAILED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SWAPCHAIN, "Failed to cast swapchain to IDXGISwapChain3\n");
			throw std::runtime_error("");
		}


		//Convert backbuffers to D3D12Textures
		m_backBuffers.resize(m_numBuffers);
		for (std::uint32_t i{ 0 }; i < m_numBuffers; ++i)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
			m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

			TextureDesc desc{};
			desc.format = DATA_FORMAT::R8G8B8A8_UNORM;
			desc.arrayTexture = false;
			desc.dimension = TEXTURE_DIMENSION::DIM_2;
			desc.size = glm::ivec3(m_surface->GetSize().x, m_surface->GetSize().y, 1);
			desc.usage = TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT;

			m_backBuffers[i] = UniquePtr<ITexture>(NK_NEW(D3D12Texture, m_logger, m_allocator, m_device, desc, backBuffer.Get()));
		}


		m_logger.Unindent();
	}



	void D3D12Swapchain::CreateRenderTargetViews()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating render target views\n");

		D3D12Device& device{ dynamic_cast<D3D12Device&>(m_device) };


		//Create descriptor heap for rtvs
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = m_numBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //doesn't need to be shader visible
		HRESULT hr{ device.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)) };
		m_rtvDescriptorSize = device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		
		//Create views
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		m_backBufferViews.resize(m_numBuffers);
		for (std::uint32_t i{ 0 }; i < m_numBuffers; ++i)
		{
			TextureViewDesc desc{};
			desc.dimension = TEXTURE_DIMENSION::DIM_2;
			desc.format = DATA_FORMAT::R8G8B8A8_UNORM;
			desc.type = TEXTURE_VIEW_TYPE::RENDER_TARGET;

			m_backBufferViews[i] = UniquePtr<ITextureView>(NK_NEW(D3D12TextureView, m_logger, m_allocator, m_device, m_backBuffers[i].get(), desc, m_rtvDescriptorHeap.Get(), m_rtvDescriptorSize, i));
		}


		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Successfully created render target views\n");
		
		m_logger.Unindent();
	}

}