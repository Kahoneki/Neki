#include "D3D12CommandBuffer.h"
#include "D3D12CommandPool.h"
#include "D3D12Device.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR //conflicts with LOGGER_CHANNEL::ERROR
#endif
#include "D3D12Texture.h"
#include "D3D12TextureView.h"
#include "D3D12Pipeline.h"

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



	void D3D12CommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthStencilAttachment)
	{
		//Colour attachments
		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
			beg.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(_colourAttachments[i].GetFormat());
			for (std::size_t i{ 0 }; i < 4; ++i) { beg.Clear.ClearValue.Color[i] = 0.0f; }
			
			colourAttachmentInfos[i].BeginningAccess = beg;
			colourAttachmentInfos[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			colourAttachmentInfos[i].cpuDescriptor = dynamic_cast<D3D12TextureView*>(&(_colourAttachments[i]))->GetCPUDescriptorHandle();
		}

		//Depth-stencil attachment
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilAttachmentInfo{};
		if (_depthStencilAttachment)
		{
			D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
			beg.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(_depthStencilAttachment->GetFormat());
			beg.Clear.ClearValue.DepthStencil.Depth = 1.0f;
			beg.Clear.ClearValue.DepthStencil.Stencil = 0;
			
			depthStencilAttachmentInfo.DepthBeginningAccess = beg;
			depthStencilAttachmentInfo.StencilBeginningAccess = beg;
			depthStencilAttachmentInfo.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			depthStencilAttachmentInfo.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			depthStencilAttachmentInfo.cpuDescriptor = dynamic_cast<D3D12TextureView*>(_depthStencilAttachment)->GetCPUDescriptorHandle();
		}

		//todo: look into allow uav write flag?
		m_buffer->BeginRenderPass(_numColourAttachments, colourAttachmentInfos.data(), _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr, D3D12_RENDER_PASS_FLAG_NONE);
	}



	void D3D12CommandBuffer::EndRendering()
	{
		m_buffer->EndRenderPass();
	}

	
	
	void D3D12CommandBuffer::BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint)
	{
		m_buffer->SetPipelineState(dynamic_cast<D3D12Pipeline*>(_pipeline)->GetPipeline());
	}



	void D3D12CommandBuffer::SetViewport(glm::vec2 _pos, glm::vec2 _extent)
	{
		//todo: look into depth members of D3D12_VIEWPORT?
		D3D12_VIEWPORT viewport{};
		viewport.TopLeftX = _pos.x;
		viewport.TopLeftY = _pos.y;
		viewport.Width = _extent.x;
		viewport.Height = _extent.y;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_buffer->RSSetViewports(1, &viewport);
	}



	void D3D12CommandBuffer::SetScissor(glm::ivec2 _pos, glm::ivec2 _extent)
	{
		//todo: maybe top-bottom should be reversed?
		D3D12_RECT scissor{};
		scissor.left = _pos.x;
		scissor.top = _pos.y;
		scissor.right = _pos.x + _extent.x;
		scissor.bottom = _pos.y + _extent.y;
		m_buffer->RSSetScissorRects(1, &scissor);
	}



	void D3D12CommandBuffer::Draw(std::uint32_t _vertexCount, std::uint32_t _instanceCount, std::uint32_t _firstVertex, std::uint32_t _firstInstance)
	{
		m_buffer->DrawInstanced(_vertexCount, _instanceCount, _firstVertex, _firstInstance);
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