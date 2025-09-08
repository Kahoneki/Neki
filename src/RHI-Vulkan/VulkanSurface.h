#pragma once
#include <RHI/ISurface.h>

namespace NK
{

	class VulkanSurface final : public ISurface
	{
	public:
		explicit VulkanSurface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SurfaceDesc& _desc);
		virtual ~VulkanSurface() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkSurfaceKHR GetSurface() const { return m_surface; }

	private:
		void CreateWindow();
		void CreateSurface();

		VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
	};

}