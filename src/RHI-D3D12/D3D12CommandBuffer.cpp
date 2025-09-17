#include "D3D12CommandBuffer.h"
#include "D3D12CommandPool.h"
#include "D3D12Device.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR //conflicts with LOGGER_CHANNEL::ERROR
#endif
#include "D3D12Texture.h"

namespace NK
{

	D3D12CommandBuffer::D3D12CommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
		: ICommandBuffer(_logger, _device, _pool, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Initialising D3D12CommandBuffer\n");

		//Create the command buffer
		D3D12_COMMAND_LIST_TYPE type;
		switch (m_pool.GetPoolType())
		{
		case COMMAND_POOL_TYPE::GRAPHICS:	type = D3D12_COMMAND_LIST_TYPE_DIRECT;	break;
		case COMMAND_POOL_TYPE::COMPUTE:	type = D3D12_COMMAND_LIST_TYPE_COMPUTE;	break;
		case COMMAND_POOL_TYPE::TRANSFER:	type = D3D12_COMMAND_LIST_TYPE_COPY;	break;
		}
		const HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommandList(0, type, dynamic_cast<D3D12CommandPool&>(m_pool).GetCommandPool(), nullptr, IID_PPV_ARGS(&m_buffer)) };

		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_BUFFER, "Initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		//Put the command list in the closed state immediately to be able to call Begin() on it and start recording
		m_buffer->Close();


		m_logger.Unindent();
	}



	D3D12CommandBuffer::~D3D12CommandBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Shutting Down D3D12CommandBuffer\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	void D3D12CommandBuffer::Reset()
	{
		//todo: implement
	}
	
	
	
	void D3D12CommandBuffer::Begin()
	{
		ID3D12CommandAllocator* allocator{ dynamic_cast<D3D12CommandPool&>(m_pool).GetCommandPool() };
		const HRESULT hr{ m_buffer->Reset(allocator, nullptr) };
		if (FAILED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Failed to reset command buffer in Begin()\n");
			throw std::runtime_error("");
		}
	}
	
	
	
	void D3D12CommandBuffer::SetBlendConstants(const float _blendConstants[4])
	{
		m_buffer->OMSetBlendFactor(_blendConstants);
	}



	void D3D12CommandBuffer::End()
	{
		const HRESULT hr{ m_buffer->Close() };
		if (FAILED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Failed to close command buffer in End()\n");
			throw std::runtime_error("");
		}
	}



	void D3D12CommandBuffer::TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
	{
		D3D12_RESOURCE_BARRIER barrierInfo{};
		barrierInfo.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierInfo.Transition.pResource = dynamic_cast<D3D12Texture*>(_texture)->GetResource();
		barrierInfo.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierInfo.Transition.StateBefore = GetD3D12ResourceState(_oldState);
		barrierInfo.Transition.StateAfter = GetD3D12ResourceState(_newState);

		m_buffer->ResourceBarrier(1, &barrierInfo);
	}



	void D3D12CommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthAttachment, ITextureView* _stencilAttachment)
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
			colourAttachmentInfos[i].clearValue.color = { {0.0f, 1.0f, 0.0f, 1.0f} };
		}

		//Depth attachment
		VkRenderingAttachmentInfo depthAttachmentInfo{};
		if (_depthAttachment)
		{
			depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachmentInfo.pNext = nullptr;
			depthAttachmentInfo.imageView = dynamic_cast<VulkanTextureView*>(_depthAttachment)->GetImageView();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
			stencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			stencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			stencilAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = dynamic_cast<VulkanTextureView*>(&(_colourAttachments[0]))->GetRenderArea();
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(_numColourAttachments);
		renderingInfo.pColorAttachments = colourAttachmentInfos.data();
		renderingInfo.pDepthAttachment = _depthAttachment ? &depthAttachmentInfo : nullptr;
		renderingInfo.pStencilAttachment = _stencilAttachment ? &stencilAttachmentInfo : nullptr;

		vkCmdBeginRendering(m_buffer, &renderingInfo);
	}



	void D3D12CommandBuffer::EndRendering()
	{
	}

	
	
	void D3D12CommandBuffer::BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint)
	{
	}



	void D3D12CommandBuffer::SetViewport(glm::vec2 _pos, glm::vec2 _extent)
	{
	}



	void D3D12CommandBuffer::SetScissor(glm::ivec2 _pos, glm::ivec2 _extent)
	{
	}



	void D3D12CommandBuffer::Draw(std::uint32_t _vertexCount, std::uint32_t _instanceCount, std::uint32_t _firstVertex, std::uint32_t _firstInstance)
	{
	}



	D3D12_RESOURCE_STATES D3D12CommandBuffer::GetD3D12ResourceState(RESOURCE_STATE _state) const
	{
		switch (_state)
		{
		case RESOURCE_STATE::UNDEFINED:			return D3D12_RESOURCE_STATE_COMMON;
		case RESOURCE_STATE::RENDER_TARGET:		return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RESOURCE_STATE::PRESENT:			return D3D12_RESOURCE_STATE_PRESENT;
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "GetD3D12ResourceState() - provided _state (" + std::to_string(std::to_underlying(_state)) + ") is not yet handled.\n");
			throw std::runtime_error("");
		}
		}
	}

}