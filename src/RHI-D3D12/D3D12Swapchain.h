#pragma once
#include <RHI/ISwapchain.h>
#include <RHI/ITexture.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12Swapchain final : public ISwapchain
	{
	public:
		explicit D3D12Swapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc);
		virtual ~D3D12Swapchain() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline IDXGISwapChain3* GetSwapchain() const { return m_swapchain.Get(); }

	private:
		void CreateSwapchain();
		void CreateRenderTargetViews();

		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		UINT m_rtvDescriptorSize{ 0 };
	};
}