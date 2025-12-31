#include "D3D12CommandBuffer.h"

#include "D3D12Buffer.h"
#include "D3D12BufferView.h"
#include "D3D12CommandPool.h"
#include "D3D12Pipeline.h"
#include "D3D12RootSignature.h"
#include "D3D12Texture.h"
#include "D3D12TextureView.h"

#include <Core/Utils/EnumUtils.h>

#include <stdexcept>


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
		case COMMAND_TYPE::GRAPHICS:	type = D3D12_COMMAND_LIST_TYPE_DIRECT;	break;
		case COMMAND_TYPE::COMPUTE:		type = D3D12_COMMAND_LIST_TYPE_COMPUTE;	break;
		case COMMAND_TYPE::TRANSFER:	type = D3D12_COMMAND_LIST_TYPE_COPY;	break;
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

	}
	
	
	
	void D3D12CommandBuffer::Begin()
	{
		ID3D12CommandAllocator* allocator{ dynamic_cast<D3D12CommandPool&>(m_pool).GetCommandPool() };
		allocator->Reset();
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



	void D3D12CommandBuffer::TransitionBarrierImpl(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
	{
		D3D12_RESOURCE_BARRIER barrierInfo{};
		barrierInfo.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierInfo.Transition.pResource = dynamic_cast<D3D12Texture*>(_texture)->GetResource();
		barrierInfo.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierInfo.Transition.StateBefore = GetD3D12ResourceState(_oldState);
		barrierInfo.Transition.StateAfter = GetD3D12ResourceState(_newState);


		if (barrierInfo.Transition.StateBefore != barrierInfo.Transition.StateAfter)
		{
			m_buffer->ResourceBarrier(1, &barrierInfo);
		}
	}



	void D3D12CommandBuffer::TransitionBarrierImpl(IBuffer* _buffer, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
	{
		if (_buffer->GetState() != _oldState)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "TransitionBarrier() - Provided _buffer's current state is " + std::to_string(std::to_underlying(_buffer->GetState())) + " which doesn't match _oldState provided (" + std::to_string(std::to_underlying(_oldState)) + ")\n");
			throw std::runtime_error("");
		}

		D3D12_RESOURCE_BARRIER barrierInfo{};
		barrierInfo.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierInfo.Transition.pResource = dynamic_cast<D3D12Buffer*>(_buffer)->GetBuffer();
		barrierInfo.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierInfo.Transition.StateBefore = (_buffer->GetMemoryType() == MEMORY_TYPE::DEVICE ? GetD3D12ResourceState(_oldState) : D3D12_RESOURCE_STATE_GENERIC_READ);
		barrierInfo.Transition.StateAfter = (_buffer->GetMemoryType() == MEMORY_TYPE::DEVICE ? GetD3D12ResourceState(_newState) : D3D12_RESOURCE_STATE_GENERIC_READ);

		//Possible because of D3D12_RESOURCE_STATE_GENERIC_READ default value for non-device memory types
		if (barrierInfo.Transition.StateBefore != barrierInfo.Transition.StateAfter)
		{
			m_buffer->ResourceBarrier(1, &barrierInfo);
		}
	}



	void D3D12CommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _multisampleDepthAttachment, ITextureView* _outputDepthAttachment, ITextureView* _stencilAttachment, bool _clearRTVs, bool _clearDSV)
	{
		if (_outputDepthAttachment && _stencilAttachment)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() called with valid _depthAttachment and _stencilAttachment. This is not supported in D3D12 - if you would like both a depth attachment and a stencil attachment, please combine them into one texture and use the other BeginRendering() function.\n");
			throw std::runtime_error("");
		}

		//Colour attachments
		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			ITextureView* activeRTV{ (_multisampleColourAttachments) ? &_multisampleColourAttachments[i] : &_outputColourAttachments[i] };

			if (_multisampleColourAttachments && _multisampleColourAttachments[i].GetParentTexture()->GetState() != RESOURCE_STATE::RENDER_TARGET)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _multisampleColourAttachments[" + std::to_string(i) + "]'s current state is " + std::to_string(std::to_underlying(_multisampleColourAttachments[i].GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::RENDER_TARGET\n");
				throw std::runtime_error("");
			}

			if (_outputColourAttachments[i].GetParentTexture()->GetState() != RESOURCE_STATE::RENDER_TARGET)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _outputColourAttachments[" + std::to_string(i) + "]'s current state is " + std::to_string(std::to_underlying(_outputColourAttachments[i].GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::RENDER_TARGET\n");
				throw std::runtime_error("");
			}

			D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
			beg.Type = (_clearRTVs ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE);
			beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(activeRTV->GetFormat());
			beg.Clear.ClearValue.Color[0] = 0.0f;
			beg.Clear.ClearValue.Color[1] = 0.0f;
			beg.Clear.ClearValue.Color[2] = 0.0f;
			beg.Clear.ClearValue.Color[3] = 1.0f;

			colourAttachmentInfos[i].BeginningAccess = beg;
			colourAttachmentInfos[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			colourAttachmentInfos[i].cpuDescriptor = dynamic_cast<D3D12TextureView*>(activeRTV)->GetCPUDescriptorHandle();
		}

		//Depth-stencil attachment
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilAttachmentInfo{};

		ITextureView* activeDSV{ _multisampleDepthAttachment ? _multisampleDepthAttachment : _outputDepthAttachment };
		if (!activeDSV && _stencilAttachment) { activeDSV = _stencilAttachment; }

		if (activeDSV)
		{
			if (_multisampleDepthAttachment && _multisampleDepthAttachment->GetParentTexture()->GetState() != RESOURCE_STATE::DEPTH_WRITE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _multisampleDepthAttachment's current state is " + std::to_string(std::to_underlying(_multisampleDepthAttachment->GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::DEPTH_WRITE\n");
				throw std::runtime_error("");
			}

			if (_outputDepthAttachment && _outputDepthAttachment->GetParentTexture()->GetState() != RESOURCE_STATE::DEPTH_WRITE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _outputDepthAttachment's current state is " + std::to_string(std::to_underlying(_outputDepthAttachment->GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::DEPTH_WRITE\n");
				throw std::runtime_error("");
			}

			if (_stencilAttachment && _stencilAttachment->GetParentTexture()->GetState() != RESOURCE_STATE::DEPTH_WRITE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _stencilAttachment's current state is " + std::to_string(std::to_underlying(_stencilAttachment->GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::DEPTH_WRITE\n");
				throw std::runtime_error("");
			}

			if (_outputDepthAttachment || _multisampleDepthAttachment)
			{
				D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
				beg.Type = (_clearDSV ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE);
				beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(activeDSV->GetFormat());
				beg.Clear.ClearValue.DepthStencil.Depth = 1.0f;
				depthStencilAttachmentInfo.DepthBeginningAccess = beg;
			}

			if (_stencilAttachment)
			{
				D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
				beg.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(_stencilAttachment->GetFormat());
				beg.Clear.ClearValue.DepthStencil.Stencil = 0;
				depthStencilAttachmentInfo.StencilBeginningAccess = beg;
				depthStencilAttachmentInfo.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			}
			else
			{
				depthStencilAttachmentInfo.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
				depthStencilAttachmentInfo.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
			}

			depthStencilAttachmentInfo.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			depthStencilAttachmentInfo.cpuDescriptor = dynamic_cast<D3D12TextureView*>(activeDSV)->GetCPUDescriptorHandle();
		}

		//todo: look into allow uav write flag?
		m_buffer->BeginRenderPass(_numColourAttachments, colourAttachmentInfos.data(), (activeDSV) ? &depthStencilAttachmentInfo : nullptr, D3D12_RENDER_PASS_FLAG_NONE);
	}



	void D3D12CommandBuffer::BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthStencilAttachment, bool _clearRTVs, bool _clearDSV)
	{
		if (_depthStencilAttachment)
		{
			if (_depthStencilAttachment->GetType() != TEXTURE_VIEW_TYPE::DEPTH_STENCIL)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() for combined depth-stencil attachments called but type of _depthStencilAttachment was not TEXTURE_VIEW_TYPE::DEPTH_STENCIL. If you are trying to use a standalone depth or stencil texture, use the other BeginRendering() function intended for this use.\n");
				throw std::runtime_error("");
			}
		}

		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> colourAttachmentInfos(_numColourAttachments);
		for (std::size_t i{ 0 }; i < _numColourAttachments; ++i)
		{
			if (_multisampleColourAttachments && _multisampleColourAttachments[i].GetParentTexture()->GetState() != RESOURCE_STATE::RENDER_TARGET)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _multisampleColourAttachments[ " + std::to_string(i) + "]'s current state is " + std::to_string(std::to_underlying(_multisampleColourAttachments[i].GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::RENDER_TARGET\n");
				throw std::runtime_error("");
			}

			if (_outputColourAttachments[i].GetParentTexture()->GetState() != RESOURCE_STATE::RENDER_TARGET)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _outputColourAttachments[" + std::to_string(i) + "]'s current state is " + std::to_string(std::to_underlying(_outputColourAttachments[i].GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::RENDER_TARGET\n");
				throw std::runtime_error("");
			}

			D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
			beg.Type = (_clearRTVs ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE);
			beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(_outputColourAttachments[i].GetFormat());
			beg.Clear.ClearValue.Color[0] = 0.0f;
			beg.Clear.ClearValue.Color[1] = 0.0f;
			beg.Clear.ClearValue.Color[2] = 0.0f;
			beg.Clear.ClearValue.Color[3] = 1.0f;

			colourAttachmentInfos[i].BeginningAccess = beg;
			colourAttachmentInfos[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			colourAttachmentInfos[i].cpuDescriptor = dynamic_cast<D3D12TextureView*>(&(_outputColourAttachments[i]))->GetCPUDescriptorHandle();
		}

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilAttachmentInfo{};
		if (_depthStencilAttachment)
		{
			if (_depthStencilAttachment->GetParentTexture()->GetState() != RESOURCE_STATE::DEPTH_WRITE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BeginRendering() - Provided _depthStencilAttachment's current state is " + std::to_string(std::to_underlying(_depthStencilAttachment->GetParentTexture()->GetState())) + " which doesn't match required state of RESOURCE_STATE::DEPTH_WRITE\n");
				throw std::runtime_error("");
			}

			D3D12_RENDER_PASS_BEGINNING_ACCESS beg{};
			beg.Type = (_clearDSV ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE);
			beg.Clear.ClearValue.Format = D3D12Texture::GetDXGIFormat(_depthStencilAttachment->GetFormat());
			beg.Clear.ClearValue.DepthStencil.Depth = 1.0f;
			beg.Clear.ClearValue.DepthStencil.Stencil = 0;

			depthStencilAttachmentInfo.DepthBeginningAccess = beg;
			depthStencilAttachmentInfo.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

			if (_depthStencilAttachment->GetFormat() == DATA_FORMAT::D24_UNORM_S8_UINT)
			{
				depthStencilAttachmentInfo.StencilBeginningAccess = beg;
				depthStencilAttachmentInfo.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			}
			else
			{
				depthStencilAttachmentInfo.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
				depthStencilAttachmentInfo.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
			}

			depthStencilAttachmentInfo.cpuDescriptor = dynamic_cast<D3D12TextureView*>(_depthStencilAttachment)->GetCPUDescriptorHandle();
		}

		m_buffer->BeginRenderPass(_numColourAttachments, colourAttachmentInfos.data(), _depthStencilAttachment ? &depthStencilAttachmentInfo : nullptr, D3D12_RENDER_PASS_FLAG_NONE);
	}



	void D3D12CommandBuffer::BlitTexture(ITexture* _srcTexture, TEXTURE_ASPECT _srcAspect, ITexture* _dstTexture, TEXTURE_ASPECT _dstAspect)
	{
		//todo: implement
		//this is actually like surprisingly hard in d3d12?? and i think you need like a whole compute shader pass....
		m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BlitTexture() - this function is not currently supported in the D3D12 backend - and hence, neither is SSAA");
		throw std::runtime_error("");
	}



	void D3D12CommandBuffer::EndRendering(std::size_t _numColourAttachments, ITexture* _multisampleColourAttachments, ITexture* _outputColourAttachments)
	{
		m_buffer->EndRenderPass();
	}



	void D3D12CommandBuffer::ResolveImage(ITexture* _multisampleTexture, ITexture* _outputTexture, TEXTURE_ASPECT _textureAspect)
	{
		//Resolve
		RESOURCE_STATE srcState{ _multisampleTexture->GetState() };
		RESOURCE_STATE dstState{ _outputTexture->GetState() };
		if (srcState != RESOURCE_STATE::RESOLVE_SOURCE)
		{
			TransitionBarrier(_multisampleTexture, srcState, RESOURCE_STATE::RESOLVE_SOURCE);
		}
		if (dstState != RESOURCE_STATE::RESOLVE_DEST)
		{
			TransitionBarrier(_outputTexture, dstState, RESOURCE_STATE::RESOLVE_DEST);
		}

		ID3D12Resource* srcTex{ dynamic_cast<D3D12Texture*>(_multisampleTexture)->GetResource() };
		ID3D12Resource* dstTex{ dynamic_cast<D3D12Texture*>(_outputTexture)->GetResource() };
		m_buffer->ResolveSubresource(dstTex, 0, srcTex, 0, D3D12Texture::GetDXGIFormat(_outputTexture->GetFormat()));

		//If we've transitioned the states, transition them back
		if (srcState != RESOURCE_STATE::RESOLVE_SOURCE)
		{
			TransitionBarrier(_multisampleTexture, RESOURCE_STATE::RESOLVE_SOURCE, srcState);
		}
		if (dstState != RESOURCE_STATE::RESOLVE_DEST)
		{
			TransitionBarrier(_outputTexture, RESOURCE_STATE::RESOLVE_DEST, dstState);
		}
	}

	
	
	void D3D12CommandBuffer::BindVertexBuffers(std::uint32_t _firstBinding, std::uint32_t _bindingCount, IBuffer* _buffers, std::size_t* _strides)
	{
		//Create the views
		std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews(_bindingCount);
		for (std::size_t i{ 0 }; i < _bindingCount; ++i)
		{
			if (_buffers[i].GetState() != RESOURCE_STATE::VERTEX_BUFFER)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BindVertexBuffers() - Provided _buffers[" + std::to_string(i) + "]'s current state is " + std::to_string(std::to_underlying(_buffers[i].GetState())) + " which doesn't match required state of RESOURCE_STATE::VERTEX_BUFFER\n");
				throw std::runtime_error("");
			}

			vertexBufferViews[i].BufferLocation = dynamic_cast<D3D12Buffer*>(&(_buffers[i]))->GetBuffer()->GetGPUVirtualAddress();
			vertexBufferViews[i].SizeInBytes = _buffers[i].GetSize();
			vertexBufferViews[i].StrideInBytes = _strides[i];
		}

		m_buffer->IASetVertexBuffers(_firstBinding, _bindingCount, vertexBufferViews.data());
	}



	void D3D12CommandBuffer::BindIndexBuffer(IBuffer* _buffer, DATA_FORMAT _format)
	{
		if (_buffer->GetState() != RESOURCE_STATE::INDEX_BUFFER)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "BindIndexBuffer() - Provided _buffer's current state is " + std::to_string(std::to_underlying(_buffer->GetState())) + " which doesn't match required state of RESOURCE_STATE::INDEX_BUFFER\n");
			throw std::runtime_error("");
		}

		//Create the view
		D3D12_INDEX_BUFFER_VIEW indexBufferView{};
		indexBufferView.BufferLocation = dynamic_cast<D3D12Buffer*>(_buffer)->GetBuffer()->GetGPUVirtualAddress();
		indexBufferView.Format = D3D12Texture::GetDXGIFormat(_format);
		indexBufferView.SizeInBytes = _buffer->GetSize();

		m_buffer->IASetIndexBuffer(&indexBufferView);
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



	void D3D12CommandBuffer::BindRootSignature(IRootSignature* _rootSignature, PIPELINE_BIND_POINT _bindPoint)
	{
		D3D12RootSignature* d3d12RootSig{ dynamic_cast<D3D12RootSignature*>(_rootSignature) };
		ID3D12DescriptorHeap* heaps[2]{ d3d12RootSig->GetResourceDescriptorHeap(), d3d12RootSig->GetSamplerDescriptorHeap() };

		//Bind root signature
		switch (_bindPoint)
		{
		case PIPELINE_BIND_POINT::GRAPHICS:
		{
			m_buffer->SetGraphicsRootSignature(d3d12RootSig->GetRootSignature());
			m_buffer->SetDescriptorHeaps(2, heaps);
			for (std::size_t i{ 0 }; i < d3d12RootSig->GetNumResourceDescriptorTables(); ++i)
			{
				m_buffer->SetGraphicsRootDescriptorTable(i, d3d12RootSig->GetResourceDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			}
			m_buffer->SetGraphicsRootDescriptorTable(d3d12RootSig->GetNumResourceDescriptorTables(), d3d12RootSig->GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			break;
		}
		case PIPELINE_BIND_POINT::COMPUTE:
		{
			m_buffer->SetComputeRootSignature(d3d12RootSig->GetRootSignature());
			m_buffer->SetDescriptorHeaps(2, heaps);
			for (std::size_t i{ 0 }; i < d3d12RootSig->GetNumResourceDescriptorTables(); ++i)
			{
				m_buffer->SetComputeRootDescriptorTable(i, d3d12RootSig->GetResourceDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			}
			m_buffer->SetComputeRootDescriptorTable(d3d12RootSig->GetNumResourceDescriptorTables(), d3d12RootSig->GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			break;
		}
		}
	}



	void D3D12CommandBuffer::PushConstants(IRootSignature* _rootSignature, void* _data)
	{
		D3D12RootSignature* d3d12RootSig{ dynamic_cast<D3D12RootSignature*>(_rootSignature) };
		switch (d3d12RootSig->GetBindPoint())
		{
		case PIPELINE_BIND_POINT::GRAPHICS: m_buffer->SetGraphicsRoot32BitConstants(d3d12RootSig->GetNumResourceDescriptorTables() + 1, d3d12RootSig->GetNum32BitValues(), _data, 0); break;
		case PIPELINE_BIND_POINT::COMPUTE: m_buffer->SetComputeRoot32BitConstants(d3d12RootSig->GetNumResourceDescriptorTables() + 1, d3d12RootSig->GetNum32BitValues(), _data, 0); break;
		}
		
	}



	void D3D12CommandBuffer::DrawIndexed(std::uint32_t _indexCount, std::uint32_t _instanceCount, std::uint32_t _firstIndex, std::uint32_t _firstInstance)
	{
		//todo: figure out a way to tie this up in another method
		m_buffer->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_buffer->DrawIndexedInstanced(_indexCount, _instanceCount, _firstIndex, 0, _firstInstance);
	}



	void D3D12CommandBuffer::CopyBufferToBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer, std::size_t _srcOffset, std::size_t _dstOffset, std::size_t _size)
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

		//Ensure source buffer is in copy source state
		if (_srcBuffer->GetState() != RESOURCE_STATE::COPY_SOURCE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "CopyBufferToBuffer() - Provided _srcBuffer's current state is " + std::to_string(std::to_underlying(_srcBuffer->GetState())) + " which doesn't match required state of RESOURCE_STATE::COPY_SOURCE\n");
			throw std::runtime_error("");
		}

		//Ensure destination buffer is in copy dest state
		if (_dstBuffer->GetState() != RESOURCE_STATE::COPY_DEST)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "CopyBufferToBuffer() - Provided _dstBuffer's current state is " + std::to_string(std::to_underlying(_dstBuffer->GetState())) + " which doesn't match required state of RESOURCE_STATE::COPY_DEST\n");
			throw std::runtime_error("");
		}


		m_buffer->CopyBufferRegion(dynamic_cast<D3D12Buffer*>(_dstBuffer)->GetBuffer(), _dstOffset, dynamic_cast<D3D12Buffer*>(_srcBuffer)->GetBuffer(), _srcOffset, _size);
	}



	void D3D12CommandBuffer::CopyBufferToTexture(IBuffer* _srcBuffer, ITexture* _dstTexture, std::size_t _srcOffset, glm::ivec3 _dstOffset, glm::ivec3 _dstExtent, std::uint32_t _mipLevel)
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

		//Ensure source buffer is in copy source state
		if (_srcBuffer->GetState() != RESOURCE_STATE::COPY_SOURCE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "CopyBufferToTexture() - Provided _srcBuffer's current state is " + std::to_string(std::to_underlying(_srcBuffer->GetState())) + " which doesn't match required state of RESOURCE_STATE::COPY_SOURCE\n");
			throw std::runtime_error("");
		}

		//Ensure destination texture is in copy dest state
		if (_dstTexture->GetState() != RESOURCE_STATE::COPY_DEST)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "CopyBufferToTexture() - Provided _dstTexture's current state is " + std::to_string(std::to_underlying(_dstTexture->GetState())) + " which doesn't match required state of RESOURCE_STATE::COPY_DEST\n");
			throw std::runtime_error("");
		}


		D3D12Buffer* d3d12SrcBuffer{ dynamic_cast<D3D12Buffer*>(_srcBuffer) };
		D3D12Texture* d3d12DstTexture{ dynamic_cast<D3D12Texture*>(_dstTexture) };
		D3D12_RESOURCE_DESC dstDesc{ d3d12DstTexture->GetResourceDesc() };

		//Calculate Subresource Index
		//For 1D/2D arrays and cubemaps: subresource = mip level + (array slice * total mip levels)
		//For 3D textures: subresource = mip level
		UINT subresourceIndex{ _mipLevel };

		UINT dstX{ static_cast<UINT>(_dstOffset.x) };
		UINT dstY{ static_cast<UINT>(_dstOffset.y) };
		UINT dstZ{ static_cast<UINT>(_dstOffset.z) };

		//If not a 3D texture, the z coordinate in _dstOffset refers to the array slice index
		//This needs to be taken into account for the subresource index, and the dst z for the copy command must be 0
		if (dstDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			UINT arraySlice{ dstZ };
			subresourceIndex = _mipLevel + (arraySlice * dstDesc.MipLevels);
			dstZ = 0;
		}

		//Get footprint
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		dynamic_cast<D3D12Device&>(m_device).GetDevice()->GetCopyableFootprints(&dstDesc, subresourceIndex, 1, _srcOffset, &footprint, nullptr, nullptr, nullptr);

		D3D12_BOX srcBox{};
		srcBox.left = 0;
		srcBox.top = 0;
		srcBox.front = 0;
		srcBox.right = footprint.Footprint.Width;
		srcBox.bottom = footprint.Footprint.Height;
		srcBox.back = footprint.Footprint.Depth;

		//Source copy location
		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = footprint;
		srcLocation.pResource = d3d12SrcBuffer->GetBuffer();

		//Destination copy location
		D3D12_TEXTURE_COPY_LOCATION dstLocation{};
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = subresourceIndex;
		dstLocation.pResource = d3d12DstTexture->GetResource();

		m_buffer->CopyTextureRegion(&dstLocation, dstX, dstY, dstZ, &srcLocation, &srcBox);
	}



	void D3D12CommandBuffer::ClearTexture(ITexture* _texture, float* _clearColour)
	{
		//todo: implement
	}



	void D3D12CommandBuffer::ClearReadWriteBufferViewImpl(IBufferView* _bufferView, std::uint32_t _clearUint)
	{
		if (_bufferView->GetType() != BUFFER_VIEW_TYPE::STORAGE_READ_WRITE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "ClearReadWriteBufferView() - Provided _bufferView is not of required type BUFFER_VIEW_TYPE::STORAGE_READ_WRITE - type = " + std::to_string(std::to_underlying(_bufferView->GetType())) + "\n");
			throw std::invalid_argument("");
		}

		D3D12Device& d3d12Device{ dynamic_cast<D3D12Device&>(m_device) };
		
		//todo: need to set the descriptor heaps back at the end of the function, this is fine for now because the only place this function is used is before BindRootSignature is called, but that's very obviously janky....
		ID3D12DescriptorHeap* heaps[]{ d3d12Device.GetResourceDescriptorHeap(), d3d12Device.GetSamplerDescriptorHeap() };
		m_buffer->SetDescriptorHeaps(2, heaps);

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ d3d12Device.GetResourceDescriptorHeap()->GetGPUDescriptorHandleForHeapStart() };
		gpuHandle.ptr += _bufferView->GetIndex() * d3d12Device.GetResourceDescriptorSize();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ d3d12Device.GetNonVisibleResourceDescriptorHeap()->GetCPUDescriptorHandleForHeapStart() };
		cpuHandle.ptr += _bufferView->GetIndex() * d3d12Device.GetResourceDescriptorSize();
		UINT clear[4]{ _clearUint, _clearUint, _clearUint, _clearUint };
		m_buffer->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, dynamic_cast<D3D12Buffer*>(_bufferView->GetParentBuffer())->GetBuffer(), clear, 0, nullptr);
	}



	void D3D12CommandBuffer::Dispatch(std::uint32_t _dimX, std::uint32_t _dimY, std::uint32_t _dimZ)
	{
		m_buffer->Dispatch(_dimX, _dimY, _dimZ);
	}




	D3D12_RESOURCE_STATES D3D12CommandBuffer::GetD3D12ResourceState(RESOURCE_STATE _state) const
	{
		switch (_state)
		{
		case RESOURCE_STATE::UNDEFINED:			    return D3D12_RESOURCE_STATE_COMMON;

		//Read-only states
		case RESOURCE_STATE::VERTEX_BUFFER:		    return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case RESOURCE_STATE::INDEX_BUFFER:		    return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case RESOURCE_STATE::CONSTANT_BUFFER:		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case RESOURCE_STATE::INDIRECT_BUFFER:		return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case RESOURCE_STATE::SHADER_RESOURCE:		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case RESOURCE_STATE::COPY_SOURCE:			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case RESOURCE_STATE::DEPTH_READ:			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case RESOURCE_STATE::RESOLVE_SOURCE:		return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

		//Read/write states
		case RESOURCE_STATE::RENDER_TARGET:		    return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RESOURCE_STATE::UNORDERED_ACCESS:		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case RESOURCE_STATE::COPY_DEST:			    return D3D12_RESOURCE_STATE_COPY_DEST;
		case RESOURCE_STATE::DEPTH_WRITE:			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RESOURCE_STATE::RESOLVE_DEST:			return D3D12_RESOURCE_STATE_RESOLVE_DEST;

		case RESOURCE_STATE::PRESENT:			    return D3D12_RESOURCE_STATE_PRESENT;

		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "GetD3D12ResourceState() - provided _state (" + std::to_string(std::to_underlying(_state)) + ") is not yet handled.\n");
			throw std::runtime_error("");
		}
		}
	}

}