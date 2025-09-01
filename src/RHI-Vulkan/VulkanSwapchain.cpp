#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanDevice.h"
#include "VulkanSurface.h"

namespace NK
{

	VulkanSwapchain::VulkanSwapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc)
	: ISwapchain(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SWAPCHAIN, "Initialising VulkanSwapchain\n");

		CreateSwapchain();
		CreateSwapchainImageViews();

		m_logger.Unindent();
	}



	VulkanSwapchain::~VulkanSwapchain()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SWAPCHAIN, "Shutting Down VulkanSwapchain\n");

		for (VkImageView iv : swapchainImageViews)
		{
			if (iv != VK_NULL_HANDLE)
			{
				vkDestroyImageView(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), iv, m_allocator.GetVulkanCallbacks());
				iv = VK_NULL_HANDLE;
			}
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Image Views Destroyed\n");

		//m_swapchainImages are owned by m_swapchain

		if (m_swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, m_allocator.GetVulkanCallbacks());
			m_swapchain = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Swapchain (and associated images) Destroyed\n");
		}

		m_logger.Unindent();
	}



	void VulkanSwapchain::CreateSwapchain()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating swapchain\n");

		const VkPhysicalDevice physicalDevice{ dynamic_cast<VulkanDevice&>(m_device).GetPhysicalDevice() };
		const VkSurfaceKHR surface{ dynamic_cast<VulkanSurface*>(m_surface)->GetSurface() };

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);


		//Get supported formats
		std::uint32_t formatCount;
		std::vector<VkSurfaceFormatKHR> formats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());


		//Choose suitable format
		VkSurfaceFormatKHR surfaceFormat{ formats[0] }; //Default to first format if ideal isn't found
		bool idealFound{ false };
		for (const VkSurfaceFormatKHR& availableFormat : formats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				idealFound = true;
				surfaceFormat = availableFormat;
				break;
			}
		}
		if (!idealFound)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, "Ideal swapchain image format (B8G8R8A8_SRGB with SRGB-nonlinear colour space) unavailable, falling back to format " + std::to_string(surfaceFormat.format) + " with colour space " + std::to_string(surfaceFormat.colorSpace) + "\n");
		}
		m_format = surfaceFormat.format;


		//Get swapchain extent
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			m_extent = capabilities.currentExtent;
		}
		else
		{
			m_extent.width = std::clamp(static_cast<std::uint32_t>(m_surface->GetSize().x), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			m_extent.height = std::clamp(static_cast<std::uint32_t>(m_surface->GetSize().y), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}


		//Get supported present modes
		std::uint32_t presentModeCount;
		std::vector<VkPresentModeKHR> presentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());


		//Choose suitable present mode
		VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR }; //Guaranteed to be available
		idealFound = false;
		for (const VkPresentModeKHR& availablePresentMode : presentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				idealFound = true;
				presentMode = availablePresentMode;
				break;
			}
		}
		if (!idealFound)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, "Ideal swapchain present mode (MAILBOX) unavailable, falling back to FIFO\n");
		}


		//Clamp desired image count
		if (m_numBuffers < capabilities.minImageCount || m_numBuffers > capabilities.maxImageCount)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, "Requested number of buffers " + std::to_string(m_numBuffers) + " falls outside of device's capabable range [" + std::to_string(capabilities.minImageCount) + ", " + std::to_string(capabilities.maxImageCount) + "] - clamping buffer count to ");
			m_numBuffers = std::clamp(m_numBuffers, capabilities.minImageCount, capabilities.maxImageCount);
			m_logger.RawLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, std::to_string(m_numBuffers) + "\n");
		}


		//Create swapchain
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = m_numBuffers;
		createInfo.imageFormat = m_format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = m_extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		const VkResult result{ vkCreateSwapchainKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &m_swapchain) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Successfully created swapchain\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SWAPCHAIN, "Failed to create swapchain - result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}


		//Get handles to swapchain images
		std::uint32_t swapchainImageCount;
		vkGetSwapchainImagesKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, &swapchainImageCount, nullptr);
		if (swapchainImageCount != m_numBuffers)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, "Number of requested buffers (" + std::to_string(m_numBuffers) + ") does not match number of swapchain images created by the device (" + std::to_string(swapchainImageCount) + "). Setting m_numBuffers to " + std::to_string(swapchainImageCount) + " accordingly\n");
			m_numBuffers = swapchainImageCount;
		}
		swapchainImages.resize(m_numBuffers);
		vkGetSwapchainImagesKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, &swapchainImageCount, swapchainImages.data());
		

		m_logger.Unindent();
	}



	void VulkanSwapchain::CreateSwapchainImageViews()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating swapchain image views\n");

		swapchainImageViews.resize(swapchainImages.size());
		for (std::size_t i{ 0 }; i < swapchainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_format;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			const VkResult result = vkCreateImageView(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &swapchainImageViews[i]);
			if (result != VK_SUCCESS)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SWAPCHAIN, "Failed to create image view " + std::to_string(i) + "/" + std::to_string(swapchainImageViews.size()) + " - result = " + std::to_string(result) + "\n");
				throw std::runtime_error("");
			}
		}

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Successfully created image views\n");
		
		m_logger.Unindent();
	}

}