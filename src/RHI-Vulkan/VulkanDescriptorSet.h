#pragma once
#include <RHI/IDescriptorSet.h>
#include "VulkanDevice.h"

namespace NK
{
	class VulkanDescriptorSet final : public IDescriptorSet
	{
	public:
		explicit VulkanDescriptorSet(VulkanDevice& _device, VkDescriptorPool _descriptorPool, VkDescriptorSet _descriptorSet);
		virtual ~VulkanDescriptorSet() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }


	private:
		//Dependency injections
		VulkanDevice& m_device;
		VkDescriptorPool m_descriptorPool;

		VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	};
}