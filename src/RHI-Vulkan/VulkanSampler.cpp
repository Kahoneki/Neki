#include "VulkanSampler.h"
#include <stdexcept>
#include <Core/Utils/FormatUtils.h>
#include "VulkanDevice.h"
#include "VulkanPipeline.h"

namespace NK
{

	VulkanSampler::VulkanSampler(ILogger& _logger, IAllocator& _allocator, FreeListAllocator& _freeListAllocator, IDevice& _device, const SamplerDesc& _desc, VkDescriptorSet _descriptorSet)
	: ISampler(_logger, _allocator, _freeListAllocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SAMPLER, "Initialising VulkanSampler\n");

		//Get resource index from free list
		m_samplerIndex = m_freeListAllocator.Allocate();
		if (m_samplerIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SAMPLER, "Resource index allocation failed - max bindless sampler count (" + std::to_string(MAX_BINDLESS_SAMPLERS) + ") reached.\n");
			throw std::runtime_error("");
		}

		
		//Create the sampler
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.minFilter = GetVulkanFilter(_desc.minFilter);
		createInfo.magFilter = GetVulkanFilter(_desc.magFilter);
		createInfo.mipmapMode = GetVulkanMipmapMode(_desc.mipmapFilter);
		createInfo.addressModeU = GetVulkanAddressMode(_desc.addressModeU);
		createInfo.addressModeV = GetVulkanAddressMode(_desc.addressModeV);
		createInfo.addressModeW = GetVulkanAddressMode(_desc.addressModeW);
		createInfo.mipLodBias = _desc.mipLODBias;
		createInfo.anisotropyEnable = _desc.maxAnisotropy > 1;
		createInfo.maxAnisotropy = _desc.maxAnisotropy;
		createInfo.compareEnable = _desc.compareOp != COMPARE_OP::ALWAYS;
		createInfo.compareOp = VulkanPipeline::GetVulkanCompareOp(_desc.compareOp);
		createInfo.minLod = _desc.minLOD;
		createInfo.maxLod = _desc.maxLOD;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		
		const VkResult result{ vkCreateSampler(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &m_sampler) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SAMPLER, "VkSampler initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SAMPLER, "VkSampler initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		//Populate descriptor info
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = m_sampler;


		//Populate write info
		VkWriteDescriptorSet writeInfo{};
		writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeInfo.dstSet = _descriptorSet;
		writeInfo.dstBinding = 1; //All bindless samplers are in binding point 0
		writeInfo.dstArrayElement = m_samplerIndex;
		writeInfo.descriptorCount = 1;
		writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writeInfo.pImageInfo = &imageInfo;


		vkUpdateDescriptorSets(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), 1, &writeInfo, 0, nullptr);


		m_logger.Unindent();
	}



	VulkanSampler::~VulkanSampler()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SAMPLER, "Shutting Down VulkanSampler\n");

		
		if (m_samplerIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_freeListAllocator.Free(m_samplerIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SAMPLER, "Sampler Index Freed\n");
			m_samplerIndex = FreeListAllocator::INVALID_INDEX;
		}
		
		if (m_sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_sampler, m_allocator.GetVulkanCallbacks());
			m_sampler = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SAMPLER, "Sampler Destroyed\n");
		}

		
		m_logger.Unindent();
	}



	VkFilter VulkanSampler::GetVulkanFilter(FILTER_MODE _filterMode)
	{
		switch (_filterMode)
		{
		case FILTER_MODE::NEAREST:	return VK_FILTER_NEAREST;
		case FILTER_MODE::LINEAR:	return VK_FILTER_LINEAR;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanSampler::GetVulkanFilter() - filter mode = " + std::to_string(std::to_underlying(_filterMode)));
		}
		}
	}



	VkSamplerMipmapMode VulkanSampler::GetVulkanMipmapMode(FILTER_MODE _filterMode)
	{
		switch (_filterMode)
		{
		case FILTER_MODE::NEAREST:	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case FILTER_MODE::LINEAR:	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanSampler::GetVulkanMipmapMode() - filter mode = " + std::to_string(std::to_underlying(_filterMode)));
		}
		}
	}



	VkSamplerAddressMode VulkanSampler::GetVulkanAddressMode(ADDRESS_MODE _addressMode)
	{
		switch (_addressMode)
		{
		case ADDRESS_MODE::REPEAT:					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case ADDRESS_MODE::MIRRORED_REPEAT:			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case ADDRESS_MODE::CLAMP_TO_EDGE:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case ADDRESS_MODE::CLAMP_TO_BORDER:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case ADDRESS_MODE::MIRROR_CLAMP_TO_EDGE:	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanSampler::GetVulkanAddressMode() - address mode = " + std::to_string(std::to_underlying(_addressMode)));
		}
		}
	}
	
}