#include "VulkanCommandBuffer.h"

#include "VulkanCommandPool.h"
#include <stdexcept>

#include "VulkanPipeline.h"
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
		//todo: implement
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
		for (std::size_t i{ 0 }; i<_numColourAttachments; ++i)
		{
			colourAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_colourAttachments[i]))->GetImageView();
			colourAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colourAttachmentInfos[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colourAttachmentInfos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachmentInfos[i].clearValue.color = { {0.0f, 1.0f, 0.0f, 1.0f} };
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



	void VulkanCommandBuffer::BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint)
	{
		switch (_bindPoint)
		{
		case PIPELINE_BIND_POINT::GRAPHICS: vkCmdBindPipeline(m_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_cast<VulkanPipeline*>(_pipeline)->GetPipeline()); break;
		case PIPELINE_BIND_POINT::COMPUTE: vkCmdBindPipeline(m_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, dynamic_cast<VulkanPipeline*>(_pipeline)->GetPipeline()); break;
		}
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



	void VulkanCommandBuffer::Draw(std::uint32_t _vertexCount, std::uint32_t _instanceCount, std::uint32_t _firstVertex, std::uint32_t _firstInstance)
	{
		vkCmdDraw(m_buffer, _vertexCount, _instanceCount, _firstVertex, _firstInstance);
	}



	BarrierInfo VulkanCommandBuffer::GetVulkanBarrierInfo(RESOURCE_STATE _state)
	{
		switch (_state)
		{
		case RESOURCE_STATE::UNDEFINED:			return { VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		case RESOURCE_STATE::RENDER_TARGET:		return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		case RESOURCE_STATE::PRESENT:			return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "GetVulkanBarrierInfo() - provided _state (" + std::to_string(std::to_underlying(_state)) + ") is not yet handled.\n");
			throw std::runtime_error("");
		}
		}
	}
}