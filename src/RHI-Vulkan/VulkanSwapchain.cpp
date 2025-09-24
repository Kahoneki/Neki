#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include <stdexcept>

#include "VulkanFence.h"
#include "VulkanSemaphore.h"
#include "VulkanQueue.h"

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


		if (m_swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, m_allocator.GetVulkanCallbacks());
			m_swapchain = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Swapchain (and associated images) Destroyed\n");
		}


		m_logger.Unindent();
	}

	
	
	std::uint32_t VulkanSwapchain::AcquireNextImageIndex(ISemaphore* _signalSemaphore, IFence* _signalFence)
	{
		//todo: look into adding VK_EXT(/KHR)_swapchain_maintance1 device extension for this
//		std::uint32_t imageIndex;
//		
//		VkAcquireNextImageInfoKHR nextImageInfo{};
//		nextImageInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
//		nextImageInfo.swapchain = m_swapchain;
//		nextImageInfo.deviceMask = 0;
//		nextImageInfo.timeout = UINT64_MAX;
//		nextImageInfo.fence = dynamic_cast<VulkanFence*>(_signalFence)->GetFence();
//		nextImageInfo.semaphore = dynamic_cast<VulkanSemaphore*>(_signalSemaphore)->GetSemaphore();
//		vkAcquireNextImage2KHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &nextImageInfo, &imageIndex);

//		return imageIndex;


		
		std::uint32_t imageIndex;
		VkSemaphore vkSignalSemaphore{ _signalSemaphore ? dynamic_cast<VulkanSemaphore*>(_signalSemaphore)->GetSemaphore() : VK_NULL_HANDLE };
		VkFence vkSignalFence{ _signalFence ? dynamic_cast<VulkanFence*>(_signalFence)->GetFence() : VK_NULL_HANDLE };
		vkAcquireNextImageKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, UINT64_MAX, vkSignalSemaphore, vkSignalFence, &imageIndex);
		return imageIndex;
	}



	void VulkanSwapchain::Present(ISemaphore* _waitSemaphore, std::uint32_t _imageIndex)
	{
		VkSemaphore vkWaitSemaphore{ _waitSemaphore ? dynamic_cast<VulkanSemaphore*>(_waitSemaphore)->GetSemaphore() : VK_NULL_HANDLE };
		
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = _waitSemaphore ? 1 : 0;
		presentInfo.pWaitSemaphores = _waitSemaphore ? &vkWaitSemaphore : nullptr;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &_imageIndex;

		vkQueuePresentKHR(dynamic_cast<VulkanQueue*>(m_presentQueue)->GetQueue(), &presentInfo);
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
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				idealFound = true;
				surfaceFormat = availableFormat;
				break;
			}
		}
		if (!idealFound)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SWAPCHAIN, "Ideal swapchain image format (R8G8B8A8_SRGB with SRGB-nonlinear colour space) unavailable, falling back to format " + std::to_string(surfaceFormat.format) + " with colour space " + std::to_string(surfaceFormat.colorSpace) + "\n");
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
		std::vector<VkImage> swapchainImages(m_numBuffers);
		vkGetSwapchainImagesKHR(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_swapchain, &swapchainImageCount, swapchainImages.data());


		//Convert swapchain images to VulkanTexture to populate m_backBuffers
		for (VkImage image : swapchainImages)
		{
			TextureDesc desc{};
			desc.size = glm::ivec3(m_extent.width, m_extent.height, 1);
			desc.arrayTexture = false;
			desc.usage = TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT;
			desc.format = VulkanTexture::GetRHIFormat(m_format);
			desc.dimension = TEXTURE_DIMENSION::DIM_2;
			m_backBuffers.push_back(UniquePtr<ITexture>(NK_NEW(VulkanTexture, m_logger, m_allocator, m_device, desc, image)));
		}

		
		m_logger.Unindent();
	}



	void VulkanSwapchain::CreateSwapchainImageViews()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating swapchain image views\n");
		
		for (std::size_t i{ 0 }; i < m_backBuffers.size(); ++i)
		{
			TextureViewDesc desc{};
			desc.dimension = TEXTURE_DIMENSION::DIM_2;
			desc.format = VulkanTexture::GetRHIFormat(m_format);
			desc.type = TEXTURE_VIEW_TYPE::RENDER_TARGET;

			m_backBufferViews.push_back(UniquePtr<ITextureView>(NK_NEW(VulkanTextureView, m_logger, m_allocator, m_device, m_backBuffers[i].get(), desc)));
		}

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SWAPCHAIN, "Successfully created image views\n");
		
		m_logger.Unindent();
	}

}