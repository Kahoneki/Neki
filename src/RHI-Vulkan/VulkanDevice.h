#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <RHI/IDevice.h>
#include <vulkan/vulkan.h>



namespace NK
{
	class VulkanDevice final : public IDevice
	{
	public:
		explicit VulkanDevice(ILogger& _logger, IAllocator& _allocator);
		virtual ~VulkanDevice() override;

		//IDevice interface implementation
//		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) override;
//		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) override;
//		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const Window* _window) override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkInstance GetInstance() const { return m_instance; }
		[[nodiscard]] inline VkDevice GetDevice() const { return m_device; }
		[[nodiscard]] inline VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
		[[nodiscard]] inline std::uint32_t GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
		[[nodiscard]] inline std::uint32_t GetComputeQueueFamilyIndex() const { return m_computeQueueFamilyIndex; }
		[[nodiscard]] inline std::uint32_t GetTransferQueueFamilyIndex() const { return m_transferQueueFamilyIndex; }

	private:
		//Init sub-methods
		void CreateInstance();
		void SetupDebugMessenger();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();

		//Utility functions
		[[nodiscard]] bool ValidationLayerSupported() const;
		[[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _info) const;
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity, VkDebugUtilsMessageTypeFlagsEXT _messageType, const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData, void* _pUserData);
		[[nodiscard]] bool PhysicalDeviceSuitable(VkPhysicalDevice _device) const; //Checks for physical device compatibility with queue, extension, and feature requirements


		//Vulkan handles
		VkInstance m_instance{ VK_NULL_HANDLE };
		VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
		VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
		VkDevice m_device{ VK_NULL_HANDLE };

		VkQueue m_graphicsQueue{ VK_NULL_HANDLE };
		std::uint32_t m_graphicsQueueFamilyIndex{ UINT32_MAX };

		VkQueue m_computeQueue{ VK_NULL_HANDLE };
		std::uint32_t m_computeQueueFamilyIndex{ UINT32_MAX };

		VkQueue m_transferQueue{ VK_NULL_HANDLE };
		std::uint32_t m_transferQueueFamilyIndex{ UINT32_MAX };


		bool m_enableInstanceValidationLayers = true;
		const std::array<const char*, 1> m_instanceValidationLayers{ "VK_LAYER_KHRONOS_validation" };
		const std::array<const char*, 1> m_requiredInstanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
		const std::array<const char*, 2> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_mesh_shader" };
	};
}