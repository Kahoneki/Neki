#include "VulkanSurface.h"

#include "VulkanDevice.h"
#include <stdexcept>

namespace NK
{

	VulkanSurface::VulkanSurface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, Window* _window)
	: ISurface(_logger, _allocator, _device, _window)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Initialising VulkanSurface\n");

		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating surface\n");

		const VkResult result{ glfwCreateWindowSurface(dynamic_cast<VulkanDevice&>(m_device).GetInstance(), m_window->GetGLFWWindow(), nullptr, &m_surface) }; //glfw does internal allocations which get buggy when combined with VkAllocationCallbacks
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SURFACE, "Surface successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SURFACE, "Failed to create surface - result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();

		m_logger.Unindent();
	}



	VulkanSurface::~VulkanSurface()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Shutting Down VulkanSurface\n");

		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(dynamic_cast<VulkanDevice&>(m_device).GetInstance(), m_surface, nullptr); //glfw does internal allocations which get buggy when combined with VkAllocationCallbacks
			m_surface = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SURFACE, "Surface Destroyed\n");
		}

		m_logger.Unindent();
	}

}