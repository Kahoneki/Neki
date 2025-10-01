#include "VulkanDescriptorSet.h"

namespace NK
{

	VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice& _device, VkDescriptorPool _descriptorPool, VkDescriptorSet _descriptorSet)
		: m_device(_device), m_descriptorPool(_descriptorPool), m_descriptorSet(_descriptorSet)
	{
	}



	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
		if (m_descriptorSet != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(m_device.GetDevice(), m_descriptorPool, 1, &m_descriptorSet);
			m_descriptorSet = VK_NULL_HANDLE;
		}
	}

}