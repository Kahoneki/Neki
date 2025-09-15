#include "VulkanDevice.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>
#include <GLFW/glfw3.h>

#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Utils/EnumUtils.h>

#include "VulkanBuffer.h"
#include "VulkanBufferView.h"
#include "VulkanCommandPool.h"
#include "VulkanFence.h"
#include "VulkanPipeline.h"
#include "VulkanQueue.h"
#include "VulkanShader.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"


namespace NK
{

	VulkanDevice::VulkanDevice(ILogger& _logger, IAllocator& _allocator)
	: IDevice(_logger, _allocator)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Initialising VulkanDevice\n");

		CreateInstance();
		SetupDebugMessenger();
		SelectPhysicalDevice();
		CreateLogicalDevice();
		CreateMutableResourceType();
		CreateDescriptorPool();
		CreateDescriptorSetLayout();
		CreateDescriptorSet();
		CreatePipelineLayout();

		m_logger.Unindent();
	}



	VulkanDevice::~VulkanDevice()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Shutting Down VulkanDevice\n");

		if (m_pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_device, m_pipelineLayout, m_allocator.GetVulkanCallbacks());
			m_pipelineLayout = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Pipeline Layout Destroyed\n");
		}
		
		if (m_descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, m_allocator.GetVulkanCallbacks());
			m_descriptorSetLayout = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Descriptor Set Layout Destroyed\n");
		}

		if (m_descriptorSet != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &m_descriptorSet);
			m_descriptorSet = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Descriptor Set Destroyed\n");
		}

		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, m_descriptorPool, m_allocator.GetVulkanCallbacks());
			m_descriptorPool = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Descriptor Pool Destroyed\n");
		}

		if (m_device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_device);
			vkDestroyDevice(m_device, m_allocator.GetVulkanCallbacks());
			m_device = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Device Destroyed\n");
		}

		if (m_debugMessenger != VK_NULL_HANDLE)
		{
			const PFN_vkDestroyDebugUtilsMessengerEXT destroyFunc{ reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT")) };
			destroyFunc(m_instance, m_debugMessenger, m_allocator.GetVulkanCallbacks());
			m_debugMessenger = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Debug Messenger Destroyed\n");
		}

		if (m_instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_instance, m_allocator.GetVulkanCallbacks());
			m_instance = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Instance Destroyed\n");
		}

		m_logger.Unindent();
	}



	UniquePtr<IBuffer> VulkanDevice::CreateBuffer(const BufferDesc& _desc)
	{
		return UniquePtr<IBuffer>(NK_NEW(VulkanBuffer, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<IBufferView> VulkanDevice::CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc)
	{
		return UniquePtr<IBufferView>(NK_NEW(VulkanBufferView, m_logger, *m_resourceIndexAllocator, *this, _buffer, _desc, m_descriptorSet));
	}



	UniquePtr<ITexture> VulkanDevice::CreateTexture(const TextureDesc& _desc)
	{
		return UniquePtr<ITexture>(NK_NEW(VulkanTexture, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ITextureView> VulkanDevice::CreateTextureView(ITexture* _texture, const TextureViewDesc& _desc)
	{
		return UniquePtr<ITextureView>(NK_NEW(VulkanTextureView, m_logger, m_allocator, *this, _texture, _desc, m_descriptorSet, m_resourceIndexAllocator.get()));
	}



	UniquePtr<ICommandPool> VulkanDevice::CreateCommandPool(const CommandPoolDesc& _desc)
	{
		return UniquePtr<ICommandPool>(NK_NEW(VulkanCommandPool, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISurface> VulkanDevice::CreateSurface(const SurfaceDesc& _desc)
	{
		return UniquePtr<ISurface>(NK_NEW(VulkanSurface, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISwapchain> VulkanDevice::CreateSwapchain(const SwapchainDesc& _desc)
	{
		return UniquePtr<ISwapchain>(NK_NEW(VulkanSwapchain, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<IShader> VulkanDevice::CreateShader(const ShaderDesc& _desc)
	{
		return UniquePtr<IShader>(NK_NEW(VulkanShader, m_logger, _desc));
	}



	UniquePtr<IPipeline> VulkanDevice::CreatePipeline(const PipelineDesc& _desc)
	{
		return UniquePtr<IPipeline>(NK_NEW(VulkanPipeline, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<IQueue> VulkanDevice::CreateQueue(const QueueDesc& _desc)
	{
		return UniquePtr<IQueue>(NK_NEW(VulkanQueue, m_logger, *this, _desc, *m_queueIndexAllocatorLookup[_desc.type]->get()));
	}



	UniquePtr<IFence> VulkanDevice::CreateFence(const FenceDesc& _desc)
	{
		return UniquePtr<IFence>(NK_NEW(VulkanFence, m_logger, m_allocator, *this, _desc));
	}



	void VulkanDevice::CreateInstance()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating instance\n");

		//Check that validation layers are available
		if (m_enableInstanceValidationLayers && !ValidationLayersSupported())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Validation layers requested, but not available.\n");
			throw std::runtime_error("");
		}

		//Setup application info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Neki App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Neki";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

		//Get required extensions
		const std::vector<const char*> extensions{ GetRequiredExtensions() };

		//Setup instance creation info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		//Point to debug messenger create info if validation is enabled
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_enableInstanceValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<std::uint32_t>(m_instanceValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_instanceValidationLayers.data();
			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		//Create the instance
		const VkResult result{ vkCreateInstance(&createInfo, m_allocator.GetVulkanCallbacks(), &m_instance) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create instance - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Instance successfully created\n");

		m_logger.Unindent();
	}



	void VulkanDevice::SetupDebugMessenger()
	{
		m_logger.Indent();

		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Setting up debug messenger\n");
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);
		const PFN_vkCreateDebugUtilsMessengerEXT createFunc{ reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT")) };
		if (createFunc(m_instance, &createInfo, m_allocator.GetVulkanCallbacks(), &m_debugMessenger) != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to set up debug messenger.\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Debug messenger successfully created\n");

		m_logger.Unindent();
	}



	void VulkanDevice::SelectPhysicalDevice()
	{
		m_logger.Indent();

		//Prioritise Discrete GPU > Integrated GPU > CPU

		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Selecting physical device\n");

		//Enumerate physical devices
		std::uint32_t physicalDeviceCount{};
		const VkResult result{ vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to enumerate physical vulkan-compatible devices - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
		VkPhysicalDeviceType physicalDeviceType{ VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM }; //Stores the best device type that has currently been found
		std::string physicalDeviceTypeString; //For logging

		for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
		{
			const VkPhysicalDevice device{ physicalDevices[i] };
			if (!PhysicalDeviceSuitable(device))
			{
				//Device doesn't meet requirements, can't be used
				//Check if this is the final device and no device has been selected yet
				if (i == physicalDeviceCount - 1 && physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM)
				{
					m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "No physical devices match requirements\n");
				}

				//Skip to next physical device
				continue;
			}

			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				m_physicalDevice = device;
				physicalDeviceTypeString = "Discrete GPU";
				break;
			}
			else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				if (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
					m_physicalDevice = device;
					physicalDeviceTypeString = "Integrated GPU";
				}
			}
			else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			{
				if ((physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
				{
					physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
					m_physicalDevice = device;
					physicalDeviceTypeString = "CPU";
				}
			}
		}

		if (physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "No suitable physical device found (searched for: Discrete GPU, Integrated GPU, CPU)\n");
			throw std::runtime_error("");
		}

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Physical device of type " + physicalDeviceTypeString + " selected\n");

		//Get queue family indexes
		std::uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
		//Also store the queue counts of each respective family to initialise the free list allocators
		std::uint32_t numGraphicsQueues;
		std::uint32_t numComputeQueues;
		std::uint32_t numTransferQueues;
		for (std::size_t i{ 0 }; i < queueFamilies.size(); ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_graphicsQueueFamilyIndex = i;
				numGraphicsQueues = queueFamilies[i].queueCount;
			}

			//todo: maybe look into changing these `else if` checks to `if` checks - this would allow a single queue family to be used for multiple purposes
			//todo: ^this also would provide support for devices with a queue family set that has, for example, a combined graphics and transfer queue family, but not a standalone transfer queue family
			//todo: ^this would probably require having multiple queues from the same queue family being used for different purposes, checking the number of queues in a family against what is required based on the configuration requires logic that would be messy based on the current setup
			else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				m_computeQueueFamilyIndex = i;
				numComputeQueues = queueFamilies[i].queueCount;
			}

			else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				m_transferQueueFamilyIndex = i;
				numTransferQueues = queueFamilies[i].queueCount;
			}
		}
		
		//Initialise the free list allocators
		m_graphicsQueueIndexAllocator = UniquePtr<FreeListAllocator>(NK_NEW(FreeListAllocator, numGraphicsQueues));
		m_computeQueueIndexAllocator = UniquePtr<FreeListAllocator>(NK_NEW(FreeListAllocator, numComputeQueues));
		m_transferQueueIndexAllocator = UniquePtr<FreeListAllocator>(NK_NEW(FreeListAllocator, numTransferQueues));
		
		
		m_logger.Unindent();
	}



	void VulkanDevice::CreateLogicalDevice()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating logical device\n");


		//Set up queue create infos
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		const std::set<std::uint32_t> uniqueQueueFamilies{ m_graphicsQueueFamilyIndex, m_computeQueueFamilyIndex, m_transferQueueFamilyIndex }; //Use a set to handle cases where graphics/compute/transfer/etc. are the same family

		std::uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

		//Get max number of queues across all families for size of queuePriorities vector
		const std::size_t maxQueues{ std::max(std::max(queueFamilies[m_graphicsQueueFamilyIndex].queueCount, queueFamilies[m_computeQueueFamilyIndex].queueCount), queueFamilies[m_transferQueueFamilyIndex].queueCount) };
		const std::vector<float> queuePriorities(maxQueues, 1.0f);

		for (std::uint32_t queueFamilyIndex : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			queueCreateInfo.queueCount = queueFamilies[queueFamilyIndex].queueCount;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			queueCreateInfos.push_back(queueCreateInfo);
		}


		//Define required features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.descriptorIndexing = VK_TRUE;
		features12.descriptorBindingPartiallyBound = VK_TRUE; //Allow empty descriptor slots - don't crash when accessing
		features12.runtimeDescriptorArray = VK_TRUE; //Enable variable-indexing
		//Allow updates to the descriptor set while the queue is using it
		features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		features12.bufferDeviceAddress = VK_TRUE;

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
		dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
		dynamicRenderingFeatures.pNext = &features12;

		VkPhysicalDeviceFeatures2 deviceFeatures2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
		deviceFeatures2.pNext = &dynamicRenderingFeatures;

		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableTypeFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
		mutableTypeFeatures.mutableDescriptorType = VK_TRUE;
		mutableTypeFeatures.pNext = &deviceFeatures2;


		//Create device
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &mutableTypeFeatures;
		deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.enabledExtensionCount = requiredDeviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

		const VkResult result{ vkCreateDevice(m_physicalDevice, &deviceCreateInfo, m_allocator.GetVulkanCallbacks(), &m_device) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create logical device - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Successfully created logical device\n");
		

		m_logger.Unindent();
	}



	void VulkanDevice::CreateMutableResourceType()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating mutable resource type\n");

		m_mutableResourceTypeList.descriptorTypeCount = m_resourceTypes.size();
		m_mutableResourceTypeList.pDescriptorTypes = m_resourceTypes.data();

		m_mutableResourceTypeInfo.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
		m_mutableResourceTypeInfo.mutableDescriptorTypeListCount = 1;
		m_mutableResourceTypeInfo.pMutableDescriptorTypeLists = &m_mutableResourceTypeList;

		m_logger.Unindent();
	}



	void VulkanDevice::CreateDescriptorPool()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating descriptor pool\n");

		//Define the pool size
		constexpr std::array<VkDescriptorPoolSize, 2> poolSizes
		{
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_MUTABLE_EXT, MAX_BINDLESS_RESOURCES },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, MAX_BINDLESS_SAMPLERS }
		};

		//Create the pool
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = &m_mutableResourceTypeInfo;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		const VkResult result{ vkCreateDescriptorPool(m_device, &poolInfo, m_allocator.GetVulkanCallbacks(), &m_descriptorPool) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create descriptor pool - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Successfully created descriptor pool\n");

		m_logger.Unindent();
	}



	void VulkanDevice::CreateDescriptorSetLayout()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating descriptor set layout\n");


		//----MUTABLE RESOURCE BINDING----//
		VkDescriptorSetLayoutBinding mutableResourceBinding{};
		mutableResourceBinding.binding = 0;
		mutableResourceBinding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
		mutableResourceBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
		mutableResourceBinding.stageFlags = VK_SHADER_STAGE_ALL;


		//----SAMPLER BINDING----//
		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.binding = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerBinding.descriptorCount = MAX_BINDLESS_SAMPLERS;
		samplerBinding.stageFlags = VK_SHADER_STAGE_ALL;


		//Create the binding chain
		constexpr std::array<VkDescriptorBindingFlags, 2> bindingFlags{ VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };
		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = 2;
		bindingFlagsInfo.pBindingFlags = bindingFlags.data();
		bindingFlagsInfo.pNext = &m_mutableResourceTypeInfo; //nothing to do with the flags, the layout info just needs the mutable type info in its chain

		const std::array<VkDescriptorSetLayoutBinding, 2> bindings{ mutableResourceBinding, samplerBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = &bindingFlagsInfo;
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		const VkResult result{ vkCreateDescriptorSetLayout(m_device, &layoutInfo, m_allocator.GetVulkanCallbacks(), &m_descriptorSetLayout) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create descriptor set layout - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Successfully created descriptor set layout\n");

		m_logger.Unindent();
	}



	void VulkanDevice::CreateDescriptorSet()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating descriptor set\n");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_descriptorSetLayout;
		const VkResult result{ vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create descriptor set - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Successfully created descriptor set\n");

		m_logger.Unindent();
	}



	void VulkanDevice::CreatePipelineLayout()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating Pipeline Layout\n");
		
		
		VkPushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
		pushConstantRange.offset = 0;
		pushConstantRange.size = 128; //128 is the minimum required by the spec

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &m_descriptorSetLayout;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushConstantRange;

		const VkResult result = vkCreatePipelineLayout(m_device, &createInfo, m_allocator.GetVulkanCallbacks(), &m_pipelineLayout);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Pipeline Layout creation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Pipeline Layout creation unsuccessful.\n");
			throw std::runtime_error("");
		}

		
		m_logger.Unindent();
	}



	bool VulkanDevice::ValidationLayersSupported() const
	{
		std::uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		//Loop through all required validation layers and check if they're available
		//Return true if all are available
		for (const char* layerName : m_instanceValidationLayers)
		{
			bool layerFound{ false };
			for (const VkLayerProperties& layerProps : availableLayers)
			{
				if (strcmp(layerName, layerProps.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
			{
				return false;
			}
		}
		return true;
	}



	std::vector<const char*> VulkanDevice::GetRequiredExtensions() const
	{
		//GLFW
		std::uint32_t glfwExtensionCount;
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		//Debug messenger
		if (m_enableInstanceValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		extensions.insert(extensions.end(), m_requiredInstanceExtensions.begin(), m_requiredInstanceExtensions.end());

		return extensions;
	}



	void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _info) const
	{
		_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		_info.pfnUserCallback = DebugCallback;
		_info.pUserData = const_cast<VulkanDevice*>(this);
	}



	VkBool32 VulkanDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity, VkDebugUtilsMessageTypeFlagsEXT _messageType, const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData, void* _pUserData)
	{
		//pUserData is the parent VulkanDevice
		const VulkanDevice* device{ reinterpret_cast<const VulkanDevice*>(_pUserData) };

		//Get channel
		LOGGER_CHANNEL channel;
		switch (_messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			channel = LOGGER_CHANNEL::INFO;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			channel = LOGGER_CHANNEL::WARNING;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			channel = LOGGER_CHANNEL::ERROR;
			break;
		default:
			channel = LOGGER_CHANNEL::NONE;
			break;
		}

		//Get layer
		LOGGER_LAYER layer;
		switch (_messageType)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			layer = LOGGER_LAYER::VULKAN_GENERAL;
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			layer = LOGGER_LAYER::VULKAN_VALIDATION;
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			layer = LOGGER_LAYER::VULKAN_PERFORMANCE;
			break;
		default:
			layer = LOGGER_LAYER::VULKAN_GENERAL;
			break;
		}

		std::string msg{ _pCallbackData->pMessage };
		if (msg.back() != '\n') { msg += '\n'; } //vulkan just like sometimes doesn't do this
		device->m_logger.IndentLog(channel, layer, msg);

		if (channel == LOGGER_CHANNEL::ERROR)
		{
			if (EnumUtils::Contains(device->m_logger.GetLoggerConfig().GetChannelBitfieldForLayer(layer), LOGGER_CHANNEL::ERROR))
			{
				throw std::runtime_error("");
			}
			else
			{
				throw std::runtime_error("Error thrown from " + device->m_logger.LayerToString(layer) + " but ERROR channel for this layer was disabled - enable it to view the error.\n");
			}
		}

		return VK_FALSE;
	}



	bool VulkanDevice::PhysicalDeviceSuitable(VkPhysicalDevice _device) const
	{
		m_logger.Indent();
		//Check that the physical device supports all required features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES, .pNext = &features12 };
		VkPhysicalDeviceFeatures2 supportedFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &dynamicRenderingFeatures };
		vkGetPhysicalDeviceFeatures2(_device, &supportedFeatures);
		if (!supportedFeatures.features.samplerAnisotropy || !dynamicRenderingFeatures.dynamicRendering || !features12.descriptorIndexing)
		{
			m_logger.Unindent();
			return false;
		}


		//Check that the physical device supports all required extensions
		std::uint32_t deviceExtensionCount;
		VkResult result{ vkEnumerateDeviceExtensionProperties(_device, nullptr, &deviceExtensionCount, nullptr) };
		if (result != VK_SUCCESS)
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to enumerate physical device features - result: " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}
		std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(_device, nullptr, &deviceExtensionCount, deviceExtensions.data());
		for (const char* extensionName : requiredDeviceExtensions)
		{
			bool extensionFound{ false };
			for (const VkExtensionProperties& ext : deviceExtensions)
			{
				if (strcmp(extensionName, ext.extensionName) == 0)
				{
					extensionFound = true;
					break;
				}
			}
			if (!extensionFound)
			{
				m_logger.Log(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::DEVICE, "Device is missing required extension: " + std::string(extensionName) + "\n");
				m_logger.Unindent();
				return false;
			}
		}


		//Check that physical device has the required queue families
		std::uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());
		bool foundGraphicsQueue{ false };
		bool foundComputeQueue{ false };
		bool foundTransferQueue{ false };
		for (std::size_t i{ 0 }; i < queueFamilies.size(); ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				foundGraphicsQueue = true;
			}

			else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				foundComputeQueue = true;
			}

			else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				foundTransferQueue = true;
			}
		}
		if (!foundGraphicsQueue || !foundComputeQueue || !foundTransferQueue)
		{
			m_logger.Unindent();
			return false;
		}

		m_logger.Unindent();
		return true;
	}

}