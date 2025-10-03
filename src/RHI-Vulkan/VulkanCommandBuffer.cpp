#include "VulkanCommandBuffer.h"

#include <stdexcept>
#include "VulkanCommandPool.h"

#include <Core/Utils/EnumUtils.h>
#include "VulkanBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanRootSignature.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"

namespace NK
{

	VulkanCommandBuffer::VulkanCommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
	: ICommandBuffer(_logger, _device, _pool, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Initialising VulkanCommandBuffer\n");

		//Create the command buffer
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = dynamic_cast<VulkanCommandPool&>(m_pool).GetCommandPool();
		allocInfo.level = (m_level == COMMAND_BUFFER_LEVEL::PRIMARY ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		const VkResult result{ vkAllocateCommandBuffers(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &allocInfo, &m_buffer) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_POOL, "Initialisation successful - level = " + std::string(m_level == COMMAND_BUFFER_LEVEL::PRIMARY ? "PRIMARY" : "SECONDARY") + '\n');
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Shutting Down VulkanCommandBuffer\n");

		if (m_buffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), dynamic_cast<VulkanCommandPool&>(m_pool).GetCommandPool(), 1, &m_buffer);
			m_buffer = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_BUFFER, "Command Buffer Destroyed\n");
		}

		m_logger.Unindent();
	}



	void VulkanCommandBuffer::Reset()
	{
		vkResetCommandBuffer(m_buffer, 0);
	}



	void VulkanCommandBuffer::Begin()
	{
		//todo: maybe add some of these flags as parameters
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(m_buffer, &beginInfo);
	}



	void VulkanCommandBuffer::SetBlendConstants(const float _blendConstants[4])
	{
		vkCmdSetBlendConstants(m_buffer, _blendConstants);
	}



	void VulkanCommandBuffer::End()
	{
		vkEndCommandBuffer(m_buffer);
	}



	void VulkanCommandBuffer::TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
	{
		const BarrierInfo src{ GetVulkanBarrierInfo(_oldState) };
		const BarrierInfo dst{ GetVulkanBarrierInfo(_newState) };

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = src.layout;
		barrier.newLayout = dst.layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dynamic_cast<VulkanTexture*>(_texture)->GetTexture();

		//Currently only colour images are supported
		//todo: add everything else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask = src.accessMask;
		barrier.dstAccessMask = dst.accessMask;

		vkCmdPipelineBarrier(m_buffer, src.stageMask, dst.stageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}



	void VulkanCommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthStencilAttachment)
	{
		//Colour attachments
		std::vector<VkRenderingAttachmentInfo> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			colourAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_colourAttachments[i]))->GetImageView();
			colourAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colourAttachmentInfos[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colourAttachmentInfos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachmentInfos[i].clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		}

		//Depth-stencil attachment
		VkRenderingAttachmentInfo depthStencilAttachmentInfo{};
		if (_depthStencilAttachment)
		{
			depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthStencilAttachmentInfo.pNext = nullptr;
			depthStencilAttachmentInfo.imageView = dynamic_cast<VulkanTextureView*>(_depthStencilAttachment)->GetImageView();
			depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthStencilAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = dynamic_cast<VulkanTextureView*>(&(_colourAttachments[0]))->GetRenderArea();
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(_numColourAttachments);
		renderingInfo.pColorAttachments = colourAttachmentInfos.data();
		renderingInfo.pDepthAttachment = _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr;
		renderingInfo.pStencilAttachment = _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr;

		vkCmdBeginRendering(m_buffer, &renderingInfo);
	}



	void VulkanCommandBuffer::EndRendering()
	{
		vkCmdEndRendering(m_buffer);
	}



	void VulkanCommandBuffer::BindVertexBuffers(std::uint32_t _firstBinding, std::uint32_t _bindingCount, IBuffer* _buffers, std::size_t* _strides)
	{
		std::vector<VkBuffer> vkBuffers(_bindingCount);
		std::vector<VkDeviceSize> vkOffsets(_bindingCount);
		for (std::size_t i{ 0 }; i < _bindingCount; ++i)
		{
			vkBuffers[i] = dynamic_cast<VulkanBuffer*>(&(_buffers[i]))->GetBuffer();
			vkOffsets[i] = 0;
		}
		vkCmdBindVertexBuffers(m_buffer, _firstBinding, _bindingCount, vkBuffers.data(), vkOffsets.data());
	}



	void VulkanCommandBuffer::BindIndexBuffer(IBuffer* _buffer, DATA_FORMAT _format)
	{
		vkCmdBindIndexBuffer(m_buffer, dynamic_cast<VulkanBuffer*>(_buffer)->GetBuffer(), 0, GetVulkanIndexType(_format));
	}



	void VulkanCommandBuffer::BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint)
	{
		switch (_bindPoint)
		{
		case PIPELINE_BIND_POINT::GRAPHICS: vkCmdBindPipeline(m_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_cast<VulkanPipeline*>(_pipeline)->GetPipeline());
			break;
		case PIPELINE_BIND_POINT::COMPUTE: vkCmdBindPipeline(m_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, dynamic_cast<VulkanPipeline*>(_pipeline)->GetPipeline());
			break;
		}
	}



	void VulkanCommandBuffer::BindRootSignature(IRootSignature* _rootSignature, PIPELINE_BIND_POINT _bindPoint)
	{
		//Bind descriptor set
		VulkanRootSignature* vkRootSig{ dynamic_cast<VulkanRootSignature*>(_rootSignature) };
		VkPipelineBindPoint bindPoint{ _bindPoint == PIPELINE_BIND_POINT::GRAPHICS ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE };
		VkDescriptorSet vkDescriptorSet{ vkRootSig->GetDescriptorSet() };
		vkCmdBindDescriptorSets(m_buffer, bindPoint, vkRootSig->GetPipelineLayout(), 0, 1, &vkDescriptorSet, 0, nullptr);
	}



	void VulkanCommandBuffer::PushConstants(IRootSignature* _rootSignature, void* _data)
	{
		VulkanRootSignature* vkRootSig{ dynamic_cast<VulkanRootSignature*>(_rootSignature) };
		//todo: replace VK_SHADER_STAGE_ALL with more precise user-defined parameter
		vkCmdPushConstants(m_buffer, vkRootSig->GetPipelineLayout(), VK_SHADER_STAGE_ALL, 0, vkRootSig->GetProvidedNum32BitValues() * 4, _data);
	}



	void VulkanCommandBuffer::SetViewport(glm::vec2 _pos, glm::vec2 _extent)
	{
		VkViewport viewport{};
		viewport.x = _pos.x;
		viewport.y = _extent.y + _pos.y; //Add height extent offset to compensate for using -_extent.y for height
		viewport.width = _extent.x;
		viewport.height = -_extent.y; //Flip the viewport for parity with D3D12's +Y up coordinate system.
		vkCmdSetViewport(m_buffer, 0, 1, &viewport);
	}



	void VulkanCommandBuffer::SetScissor(glm::ivec2 _pos, glm::ivec2 _extent)
	{
		VkRect2D scissor{};
		scissor.offset.x = _pos.x;
		scissor.offset.y = _pos.y;
		scissor.extent.width = _extent.x;
		scissor.extent.height = _extent.y;
		vkCmdSetScissor(m_buffer, 0, 1, &scissor);
	}



	void VulkanCommandBuffer::DrawIndexed(std::uint32_t _indexCount, std::uint32_t _instanceCount, std::uint32_t _firstIndex, std::uint32_t _firstInstance)
	{
		vkCmdDrawIndexed(m_buffer, _indexCount, _instanceCount, _firstIndex, 0, _firstInstance);
	}



	void VulkanCommandBuffer::CopyBufferToBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer)
	{
		//Input validation

		//Ensure source buffer is transfer src capable
		if (!EnumUtils::Contains(_srcBuffer->GetUsageFlags(), NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToBuffer() - _srcBuffer wasn't created with NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT\n");
			throw std::runtime_error("");
		}

		//Ensure destination buffer is transfer dst capable
		if (!EnumUtils::Contains(_dstBuffer->GetUsageFlags(), NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToBuffer() - _dstBuffer wasn't created with NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT\n");
			throw std::runtime_error("");
		}

		//Ensure destination buffer is large enough to hold the contents of the source buffer
		if (_dstBuffer->GetSize() < _srcBuffer->GetSize())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToBuffer() - _srcBuffer (size: " + std::to_string(_srcBuffer->GetSize()) + ") is too big for _dstBuffer (size:" + std::to_string(_dstBuffer->GetSize()) + "\n");
			throw std::runtime_error("");
		}

		//Enqueue the copy command
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _srcBuffer->GetSize();
		vkCmdCopyBuffer(m_buffer, dynamic_cast<VulkanBuffer*>(_srcBuffer)->GetBuffer(), dynamic_cast<VulkanBuffer*>(_dstBuffer)->GetBuffer(), 1, &copyRegion);
	}



	void VulkanCommandBuffer::CopyBufferToTexture(IBuffer* _srcBuffer, ITexture* _dstTexture)
	{
		//Input validation

		//Ensure source buffer is transfer src capable
		if (!EnumUtils::Contains(_srcBuffer->GetUsageFlags(), NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToTexture() - _srcBuffer wasn't created with NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT\n");
			throw std::runtime_error("");
		}

		//Ensure destination texture is transfer dst capable
		if (!EnumUtils::Contains(_dstTexture->GetUsageFlags(), NK::TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToTexture() - _dstTexture wasn't created with NK::TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT\n");
			throw std::runtime_error("");
		}

		//Enqueue the copy command
		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;   //Tightly packed
		copyRegion.bufferImageHeight = 0; //Tightly packed
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset = { 0, 0, 0 };
		copyRegion.imageExtent = { static_cast<std::uint32_t>(_dstTexture->GetSize().x), static_cast<std::uint32_t>(_dstTexture->GetSize().y), static_cast<std::uint32_t>(_dstTexture->GetSize().z) };

		vkCmdCopyBufferToImage(m_buffer, dynamic_cast<VulkanBuffer*>(_srcBuffer)->GetBuffer(), dynamic_cast<VulkanTexture*>(_dstTexture)->GetTexture(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}



	void VulkanCommandBuffer::UploadDataToDeviceBuffer(void* data, std::size_t size, IBuffer* _dstBuffer)
	{
		//Todo: implement
	}



	BarrierInfo VulkanCommandBuffer::GetVulkanBarrierInfo(RESOURCE_STATE _state)
	{
		switch (_state)
		{
		case RESOURCE_STATE::UNDEFINED:			return { VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
			
		case RESOURCE_STATE::SHADER_RESOURCE:	return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		case RESOURCE_STATE::RENDER_TARGET:		return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		case RESOURCE_STATE::PRESENT:			return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

		case RESOURCE_STATE::COPY_SOURCE:		return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		case RESOURCE_STATE::COPY_DEST:			return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };

		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "GetVulkanBarrierInfo() - provided _state (" + std::to_string(std::to_underlying(_state)) + ") is not yet handled.\n");
			throw std::runtime_error("");
		}
		}
	}



	VkIndexType VulkanCommandBuffer::GetVulkanIndexType(DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::R8_UINT: return VK_INDEX_TYPE_UINT8;
		case DATA_FORMAT::R16_UINT: return VK_INDEX_TYPE_UINT16;
		case DATA_FORMAT::R32_UINT: return VK_INDEX_TYPE_UINT32;
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Default case reached in GetVulkanIndexType() - _format = " + std::to_string(std::to_underlying(_format)) + "\n");
			throw std::runtime_error("");
		}
		}
	}



	VkPipelineBindPoint VulkanCommandBuffer::GetVulkanPipelineBindPoint(PIPELINE_BIND_POINT _bindPoint)
	{
		switch (_bindPoint)
		{
		case PIPELINE_BIND_POINT::GRAPHICS: return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case PIPELINE_BIND_POINT::COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Default case reached in GetVulkanPipelineBindPoint() - _bindPoint = " + std::to_string(std::to_underlying(_bindPoint)) + "\n");
			throw std::runtime_error("");
		}
		}
	}

}