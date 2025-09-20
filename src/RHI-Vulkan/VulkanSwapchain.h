#pragma once
#include <RHI/ISwapchain.h>

namespace NK
{
	class VulkanSwapchain final : public ISwapchain
	{
	public:
		explicit VulkanSwapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc);
		virtual ~VulkanSwapchain() override;

		//Acquire the index of the next image in the swapchain - signals _signalSemaphore when the image is ready to be rendered to.
		virtual std::uint32_t AcquireNextImageIndex(ISemaphore* _signalSemaphore, IFence* _signalFence) override;

		//Presents image with index _imageIndex to the screen - waits for _waitSemaphore before presenting
		virtual void Present(ISemaphore* _waitSemaphore, std::uint32_t _imageIndex) override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkSwapchainKHR GetSwapchain() const { return m_swapchain; }


	private:
		void CreateSwapchain();
		void CreateSwapchainImageViews();


		VkExtent2D m_extent{ 0, 0 };

		VkFormat m_format{ VK_FORMAT_UNDEFINED };
		VkSwapchainKHR m_swapchain{ VK_NULL_HANDLE };
	};
}