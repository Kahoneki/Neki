#pragma once
#include <RHI/ISwapchain.h>

namespace NK
{
	class VulkanSwapchain final : public ISwapchain
	{
	public:
		explicit VulkanSwapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc);
		virtual ~VulkanSwapchain() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkSwapchainKHR GetSwapchain() const { return m_swapchain; }

	private:
		void CreateSwapchain();
		void CreateSwapchainImageViews();

		VkFormat m_format{ VK_FORMAT_UNDEFINED };
		VkSwapchainKHR m_swapchain{ VK_NULL_HANDLE };
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
	};
}