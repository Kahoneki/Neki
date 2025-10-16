#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanPipeline.h"
#include "VulkanRootSignature.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include <Core/Utils/EnumUtils.h>

#include <stdexcept>



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
		const VulkanBarrierInfo src{ VulkanUtils::GetVulkanBarrierInfo(_oldState) };
		const VulkanBarrierInfo dst{ VulkanUtils::GetVulkanBarrierInfo(_newState) };

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = src.layout;
		barrier.newLayout = dst.layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dynamic_cast<VulkanTexture*>(_texture)->GetTexture();

		const DATA_FORMAT format = _texture->GetFormat();
		if (format == DATA_FORMAT::D32_SFLOAT || format == DATA_FORMAT::D16_UNORM)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (format == DATA_FORMAT::D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = _texture->IsArrayTexture() ? (_texture->GetDimension() == TEXTURE_DIMENSION::DIM_1 ? _texture->GetSize().y : _texture->GetSize().z) : 1;;

		barrier.srcAccessMask = src.accessMask;
		barrier.dstAccessMask = dst.accessMask;

		vkCmdPipelineBarrier(m_buffer, src.stageMask, dst.stageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}



	void VulkanCommandBuffer::TransitionBarrier(IBuffer* _buffer, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
	{
		const VulkanBarrierInfo src{ VulkanUtils::GetVulkanBarrierInfo(_oldState) };
		const VulkanBarrierInfo dst{ VulkanUtils::GetVulkanBarrierInfo(_newState) };

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = dynamic_cast<VulkanBuffer*>(_buffer)->GetBuffer();

		barrier.offset = 0;
		barrier.size = _buffer->GetSize();

		barrier.srcAccessMask = src.accessMask;
		barrier.dstAccessMask = dst.accessMask;

		vkCmdPipelineBarrier(m_buffer, src.stageMask, dst.stageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	}



	void VulkanCommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthAttachment, ITextureView* _stencilAttachment)
	{
		if (_depthAttachment && _stencilAttachment)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() called with valid _depthAttachment and _stencilAttachment. This is not supported for parity with D3D12 - if you would like both a depth attachment and a stencil attachment, please combine them into one texture and use the other BeginRendering() function.\n");
			throw std::runtime_error("");
		}

		//Colour attachments
		std::vector<VkRenderingAttachmentInfo> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			colourAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colourAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (_multisampleColourAttachments)
			{
				//Multisampling enabled, render into _multisampleColourAttachments and resolve into _outputColourAttachments
				colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_multisampleColourAttachments[i]))->GetImageView();
				colourAttachmentInfos[i].resolveImageView = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[i]))->GetImageView();
				colourAttachmentInfos[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colourAttachmentInfos[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			else
			{
				//Multisampling disabled, ignore _multisampleColourAttachments and just render straight into _outputColourAttachments
				colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[i]))->GetImageView();
			}

			colourAttachmentInfos[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colourAttachmentInfos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachmentInfos[i].clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		}

		//Depth attachment
		VkRenderingAttachmentInfo depthAttachmentInfo{};
		if (_depthAttachment)
		{
			depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachmentInfo.pNext = nullptr;
			depthAttachmentInfo.imageView = dynamic_cast<VulkanTextureView*>(_depthAttachment)->GetImageView();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };
		}

		//Stencil attachment
		VkRenderingAttachmentInfo stencilAttachmentInfo{};
		if (_stencilAttachment)
		{
			stencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			stencilAttachmentInfo.pNext = nullptr;
			stencilAttachmentInfo.imageView = dynamic_cast<VulkanTextureView*>(_stencilAttachment)->GetImageView();
			stencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			stencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			stencilAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[0]))->GetRenderArea();
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(_numColourAttachments);
		renderingInfo.pColorAttachments = colourAttachmentInfos.data();
		renderingInfo.pDepthAttachment = _depthAttachment ? &depthAttachmentInfo : nullptr;
		renderingInfo.pStencilAttachment = _stencilAttachment ? &stencilAttachmentInfo : nullptr;

		vkCmdBeginRendering(m_buffer, &renderingInfo);
	}



	void VulkanCommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthStencilAttachment)
	{
		if (_depthStencilAttachment)
		{
			if (_depthStencilAttachment->GetType() != TEXTURE_VIEW_TYPE::DEPTH_STENCIL)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() for combined depth-stencil attachments called but type of _depthStencilAttachment was not TEXTURE_VIEW_TYPE::DEPTH_STENCIL. If you are trying to use a standalone depth or stencil texture, use the other BeginRendering() function intended for this use.\n");
				throw std::runtime_error("");
			}
		}

		//Colour attachments
		std::vector<VkRenderingAttachmentInfo> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			colourAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colourAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (_multisampleColourAttachments)
			{
				//Multisampling enabled, render into _multisampleColourAttachments and resolve into _outputColourAttachments
				colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_multisampleColourAttachments[i]))->GetImageView();
				colourAttachmentInfos[i].resolveImageView = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[i]))->GetImageView();
				colourAttachmentInfos[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colourAttachmentInfos[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			else
			{
				//Multisampling disabled, ignore _multisampleColourAttachments and just render straight into _outputColourAttachments
				colourAttachmentInfos[i].imageView = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[i]))->GetImageView();
			}

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
		renderingInfo.renderArea = dynamic_cast<VulkanTextureView*>(&(_outputColourAttachments[0]))->GetRenderArea();
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(_numColourAttachments);
		renderingInfo.pColorAttachments = colourAttachmentInfos.data();
		renderingInfo.pDepthAttachment = _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr;
		renderingInfo.pStencilAttachment = _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr;

		vkCmdBeginRendering(m_buffer, &renderingInfo);
	}



	void VulkanCommandBuffer::BlitTexture(ITexture* _srcTexture, TEXTURE_ASPECT _srcAspect, ITexture* _dstTexture, TEXTURE_ASPECT _dstAspect)
	{
		const glm::ivec3 srcSize{ _srcTexture->GetSize() };
		const glm::ivec3 dstSize{ _dstTexture->GetSize() };

		VkImageBlit blitRegion{};

		blitRegion.srcSubresource.aspectMask = VulkanUtils::GetVulkanImageAspectFlags(_srcAspect);
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = { srcSize.x, srcSize.y, srcSize.z };

		blitRegion.dstSubresource.aspectMask = VulkanUtils::GetVulkanImageAspectFlags(_dstAspect);
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = { dstSize.x, dstSize.y, dstSize.z };

		vkCmdBlitImage(m_buffer, dynamic_cast<VulkanTexture*>(_srcTexture)->GetTexture(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dynamic_cast<VulkanTexture*>(_dstTexture)->GetTexture(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
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
		vkCmdBindIndexBuffer(m_buffer, dynamic_cast<VulkanBuffer*>(_buffer)->GetBuffer(), 0, VulkanUtils::GetVulkanIndexType(_format));
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
		VkShaderStageFlags stage{ static_cast<VkShaderStageFlags>(vkRootSig->GetBindPoint() == PIPELINE_BIND_POINT::GRAPHICS ? VK_SHADER_STAGE_ALL_GRAPHICS : VK_SHADER_STAGE_COMPUTE_BIT) };
		vkCmdPushConstants(m_buffer, vkRootSig->GetPipelineLayout(), stage, 0, vkRootSig->GetProvidedNum32BitValues() * 4, _data);
	}



	void VulkanCommandBuffer::SetViewport(glm::vec2 _pos, glm::vec2 _extent)
	{
		VkViewport viewport{};
		viewport.x = _pos.x;
		viewport.y = _extent.y + _pos.y; //Add height extent offset to compensate for using -_extent.y for height
		viewport.width = _extent.x;
		viewport.height = -_extent.y; //Flip the viewport for parity with D3D12's +Y up coordinate system.
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
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



	void VulkanCommandBuffer::CopyBufferToBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer, std::size_t _srcOffset, std::size_t _dstOffset, std::size_t _size)
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

		//Ensure source buffer with given offset and size doesn't exceed bounds of source buffer
		if (_srcOffset + _size > _srcBuffer->GetSize())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToBuffer() - _srcOffset (" + std::to_string(_srcOffset) + ") + _size (" + std::to_string(_size) + ") > _srcBuffer.m_size (" + std::to_string(_srcBuffer->GetSize()) + ")\n");
			throw std::runtime_error("");
		}

		//Ensure dest buffer with given offset and size doesn't exceed bounds of dest buffer
		if (_dstOffset + _size > _dstBuffer->GetSize())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToBuffer() - _dstOffset (" + std::to_string(_dstOffset) + ") + _size (" + std::to_string(_size) + ") > _dstBuffer.m_size (" + std::to_string(_dstBuffer->GetSize()) + ")\n");
			throw std::runtime_error("");
		}

		//Enqueue the copy command
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = _srcOffset;
		copyRegion.dstOffset = _dstOffset;
		copyRegion.size = _size;
		vkCmdCopyBuffer(m_buffer, dynamic_cast<VulkanBuffer*>(_srcBuffer)->GetBuffer(), dynamic_cast<VulkanBuffer*>(_dstBuffer)->GetBuffer(), 1, &copyRegion);
	}



	void VulkanCommandBuffer::CopyBufferToTexture(IBuffer* _srcBuffer, ITexture* _dstTexture, std::size_t _srcOffset, glm::ivec3 _dstOffset, glm::ivec3 _dstExtent)
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

		//Ensure destination region doesn't exceed destination texture's bounds
		const glm::ivec3 textureSize = _dstTexture->GetSize();
		if ((_dstOffset.x + _dstExtent.x > textureSize.x) ||
		    (_dstOffset.y + _dstExtent.y > textureSize.y) ||
		    (_dstOffset.z + _dstExtent.z > textureSize.z))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "In CopyBufferToTexture() - Specified destination region is out of bounds for the texture.\n");
			throw std::runtime_error("");
		}

		//Enqueue the copy command
		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = _srcOffset;
		copyRegion.bufferRowLength = 0;   //Tightly packed
		copyRegion.bufferImageHeight = 0; //Tightly packed
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = _dstTexture->IsArrayTexture() ? (_dstTexture->GetDimension() == TEXTURE_DIMENSION::DIM_1 ? _dstTexture->GetSize().y : _dstTexture->GetSize().z) : 1;
		copyRegion.imageOffset = { _dstOffset.x, _dstOffset.y, _dstOffset.z };
		copyRegion.imageExtent = { static_cast<std::uint32_t>(_dstExtent.x), static_cast<std::uint32_t>(_dstExtent.y), static_cast<std::uint32_t>(_dstExtent.z) };

		vkCmdCopyBufferToImage(m_buffer, dynamic_cast<VulkanBuffer*>(_srcBuffer)->GetBuffer(), dynamic_cast<VulkanTexture*>(_dstTexture)->GetTexture(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

}