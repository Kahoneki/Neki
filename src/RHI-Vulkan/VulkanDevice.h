#pragma once

#include "RHI/IQueue.h"

#include <RHI/IDevice.h>

#include <array>
#include <cstdint>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>


namespace NK
{
	
	class VulkanDevice final : public IDevice
	{
	public:
		explicit VulkanDevice(ILogger& _logger, IAllocator& _allocator);
		virtual ~VulkanDevice() override;

		//IDevice interface implementation
		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IBufferView> CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateShaderResourceTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateDepthStencilTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateRenderTargetTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISampler> CreateSampler(const SamplerDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(Window* _window) override;
		[[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IRootSignature> CreateRootSignature(const RootSignatureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IQueue> CreateQueue(const QueueDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IFence> CreateFence(const FenceDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISemaphore> CreateSemaphore() override;
		[[nodiscard]] virtual UniquePtr<GPUUploader> CreateGPUUploader(const GPUUploaderDesc& _desc) override;

		[[nodiscard]] virtual TextureCopyMemoryLayout GetRequiredMemoryLayoutForTextureCopy(ITexture* _texture) override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkInstance GetInstance() const { return m_instance; }
		[[nodiscard]] inline VkDevice GetDevice() const { return m_device; }
		[[nodiscard]] inline VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
		[[nodiscard]] inline std::uint32_t GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
		[[nodiscard]] inline std::uint32_t GetComputeQueueFamilyIndex() const { return m_computeQueueFamilyIndex; }
		[[nodiscard]] inline std::uint32_t GetTransferQueueFamilyIndex() const { return m_transferQueueFamilyIndex; }
		[[nodiscard]] inline VmaAllocator GetVMAAllocator() const { return m_vmaAllocator; }
		[[nodiscard]] inline VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
		[[nodiscard]] inline VkDescriptorSetLayout GetGlobalDescriptorSetLayout() const { return m_globalDescriptorSetLayout; }
		[[nodiscard]] inline VkDescriptorSet GetGlobalDescriptorSet() const { return m_globalDescriptorSet; }

		//Public utility
		void LogVRAMUsage_Fast() const;
		void LogVRAMUsage_Detailed() const;
		

	private:
		//Init sub-methods
		void CreateInstance();
		void SetupDebugMessenger();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateVMAAllocator();
		void CreateMutableResourceType();
		void CreateDescriptorPool();
		void CreateDescriptorSetLayout();
		void CreateDescriptorSet();

		//Utility functions
		[[nodiscard]] bool ValidationLayersSupported() const;
		[[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _info) const;
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity, VkDebugUtilsMessageTypeFlagsEXT _messageType, const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData, void* _pUserData);
		[[nodiscard]] bool PhysicalDeviceSuitable(VkPhysicalDevice _device) const; //Checks for physical device compatibility with queue, extension, and feature requirements
		
		

		//Vulkan handles
		VkInstance m_instance{ VK_NULL_HANDLE };
		VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
		VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
		VkDevice m_device{ VK_NULL_HANDLE };

		std::uint32_t m_graphicsQueueFamilyIndex{ UINT32_MAX };
		UniquePtr<FreeListAllocator> m_graphicsQueueIndexAllocator;

		std::uint32_t m_computeQueueFamilyIndex{ UINT32_MAX };
		UniquePtr<FreeListAllocator> m_computeQueueIndexAllocator;

		std::uint32_t m_transferQueueFamilyIndex{ UINT32_MAX };
		UniquePtr<FreeListAllocator> m_transferQueueIndexAllocator;

		std::unordered_map<COMMAND_TYPE, UniquePtr<FreeListAllocator>*> m_queueIndexAllocatorLookup
		{
			{ COMMAND_TYPE::GRAPHICS, &m_graphicsQueueIndexAllocator },
			{ COMMAND_TYPE::COMPUTE, &m_computeQueueIndexAllocator },
			{ COMMAND_TYPE::TRANSFER, &m_transferQueueIndexAllocator },
		};

		//Mutable type
		inline static constexpr std::array<VkDescriptorType, 4> m_resourceTypes
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		};
		VkMutableDescriptorTypeListEXT m_mutableResourceTypeList{};
		VkMutableDescriptorTypeCreateInfoEXT m_mutableResourceTypeInfo{};
		VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout m_globalDescriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet m_globalDescriptorSet{ VK_NULL_HANDLE };
		
		bool m_enableInstanceValidationLayers = true;
		const std::array<const char*, 1> m_instanceValidationLayers{ "VK_LAYER_KHRONOS_validation" };
		const std::array<const char*, 1> m_requiredInstanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
		const std::array<const char*, 4> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_mesh_shader", "VK_EXT_mutable_descriptor_type", "VK_KHR_shader_non_semantic_info" };

		VmaAllocator m_vmaAllocator;
	};
	
}