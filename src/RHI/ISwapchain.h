#pragma once
#include "IDevice.h"
#include "ISurface.h"
#include "ITextureView.h"
#include "IQueue.h"

namespace NK
{


	struct SwapchainDesc
	{
		ISurface* surface;
		std::uint32_t numBuffers;
		IQueue* presentQueue;
	};
	
	
	class ISwapchain
	{
	public:
		virtual ~ISwapchain() = default;

		//Acquire the index of the next image in the swapchain - signals _signalSemaphore and _signalFence when the image is ready to be rendered to.
		virtual std::uint32_t AcquireNextImageIndex(ISemaphore* _signalSemaphore, IFence* _signalFence) = 0;

		[[nodiscard]] inline ITexture* GetImage(std::uint32_t _index) { return m_backBuffers[_index].get(); }
		[[nodiscard]] inline ITextureView* GetImageView(uint32_t _index) { return m_backBufferViews[_index].get(); }

		//Presents image with index _imageIndex to the screen - waits for _waitSemaphore before presenting
		virtual void Present(ISemaphore* _waitSemaphore, std::uint32_t _imageIndex) = 0;


	protected:
		explicit ISwapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_surface(_desc.surface), m_numBuffers(_desc.numBuffers), m_presentQueue(_desc.presentQueue) {}
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		ISurface* m_surface;
		std::uint32_t m_numBuffers;
		IQueue* m_presentQueue;

		std::vector<UniquePtr<ITexture>> m_backBuffers;
		std::vector<UniquePtr<ITextureView>> m_backBufferViews;
	};
	
}