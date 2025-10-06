#include "VulkanRootSignature.h"
#include <stdexcept>
#include <Core/Utils/FormatUtils.h>
#include "VulkanDevice.h"

namespace NK
{

	VulkanRootSignature::VulkanRootSignature(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const RootSignatureDesc& _desc)
	: IRootSignature(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ROOT_SIGNATURE, "Initialising VulkanRootSignature\n");

		
		m_descriptorSet = dynamic_cast<VulkanDevice&>(m_device).GetGlobalDescriptorSet();
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::ROOT_SIGNATURE, "Global descriptor set pulled from VulkanDevice\n");

		
		//Create the pipeline layout
		VkPushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = static_cast<VkShaderStageFlags>(m_bindPoint == PIPELINE_BIND_POINT::GRAPHICS ? VK_SHADER_STAGE_ALL_GRAPHICS : VK_SHADER_STAGE_COMPUTE_BIT);
		pushConstantRange.offset = 0;
		pushConstantRange.size = std::max(m_providedNum32BitPushConstantValues * 4, 128u); //128 bytes is the minimum required by the spec
		if (m_providedNum32BitPushConstantValues * 4 < 128u)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::ROOT_SIGNATURE, "Push constant range size increased from " + std::to_string(m_providedNum32BitPushConstantValues * 4) + " bytes to 128 bytes (minimum required by Vulkan) - this will not have an impact on your shader code.\n");
		}
		m_actualNum32BitPushConstantValues = pushConstantRange.size / 4;

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 1;
		VkDescriptorSetLayout descriptorSetLayout{ dynamic_cast<VulkanDevice&>(m_device).GetGlobalDescriptorSetLayout() };
		createInfo.pSetLayouts = &descriptorSetLayout;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushConstantRange;

		const VkResult result = vkCreatePipelineLayout(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &m_pipelineLayout);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "VkPipelineLayout initialisation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "VkPipelineLayout initialisation unsuccessful. Result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	VulkanRootSignature::~VulkanRootSignature()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ROOT_SIGNATURE, "Shutting Down VulkanRootSignature\n");

		
		if (m_pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_pipelineLayout, m_allocator.GetVulkanCallbacks());
			m_pipelineLayout = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Pipeline Layout Destroyed\n");
		}

		//m_descriptorSet is owned by the VulkanDevice

		
		m_logger.Unindent();
	}
	
}