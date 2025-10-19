#pragma once

#include "ICommandPool.h"
#include "ITexture.h"
#include "IBuffer.h"

#include <Types/NekiTypes.h>


namespace NK
{
	
	struct CommandBufferDesc
	{
		COMMAND_BUFFER_LEVEL level;
	};


	class ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer() = default;

		virtual void Reset() = 0;

		virtual void Begin() = 0;
		virtual void SetBlendConstants(const float _blendConstants[4]) = 0;
		virtual void End() = 0;


		//----NVIs----/
		//NVIs because m_state is a private member of ITexture and IBuffer - ICommandBuffer is a friend class but VulkanCommandBuffer and D3D12CommandBuffer aren't
		
		void TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
		{
			if (_texture->m_state != _oldState)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "TransitionBarrier() - Provided _texture's current state is " + std::to_string(std::to_underlying(_texture->GetState())) + " which doesn't match _oldState provided (" + std::to_string(std::to_underlying(_oldState)) + ")\n");
				throw std::runtime_error("");
			}
			TransitionBarrierImpl(_texture, _oldState, _newState);
			_texture->m_state = _newState;
		}

		void TransitionBarrier(IBuffer* _buffer, RESOURCE_STATE _oldState, RESOURCE_STATE _newState)
		{
			if (_buffer->GetState() != _oldState)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "TransitionBarrier() - Provided _buffer's current state is " + std::to_string(std::to_underlying(_buffer->GetState())) + " which doesn't match _oldState provided (" + std::to_string(std::to_underlying(_oldState)) + ")\n");
				throw std::runtime_error("");
			}
			TransitionBarrierImpl(_buffer, _oldState, _newState);
			_buffer->m_state = _newState;
		}

		//----End of NVIs----//


		//Used for individual depth and stencil attachments (either or both can be set to nullptr if unused)
		//Note: _numColourAttachments is not the size of the swapchain, it is used for rendering to multiple colour attachments in a single pass. For most scenarios, this value should be 1.
		//
		//If not using multisampling:
		//- Leave _multiSampleColourAttachments as nullptr
		//- Populate _outputColourAttachments with the final output attachment(s) with sample count = 1 (probably from ISwapchain::GetImageView())
		//
		//If using multisampling:
		//- There should be as many _multisampleColourAttachments as _outputColourAttachments (_numColourAttachments)
		//- _multisampleColourAttachments should be the offscreen attachment(s) with sample count > 1
		//- _outputColourAttachments should be the final output attachment(s) with sample count = 1 (probably from ISwapchain::GetImageView())
		//- _depthAttachment and _stencilAttachment must have a sample count equal to that of the _multisampleColourAttachment(s)
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthAttachment, ITextureView* _stencilAttachment) = 0;

		//Used for combined depth-stencil attachments (_depthStencilAttachment, if provided, must be in a combined depth-stencil compatible data format) (_depthStencilAttachment can be set to nullptr if unused)
		//Note: _numColourAttachments is not the size of the swapchain, it is used for rendering to multiple colour attachments in a single pass. For most scenarios, this value should be 1.
		//
		//If not using multisampling:
		//- Leave _multiSampleColourAttachments as nullptr
		//- Populate _outputColourAttachments with the final output attachment(s) with sample count = 1 (probably from ISwapchain::GetImageView())
		//
		//If using multisampling:
		//- There should be as many _multisampleColourAttachments as _outputColourAttachments (_numColourAttachments)
		//- _multisampleColourAttachments should be the offscreen attachment(s) with sample count > 1
		//- _outputColourAttachments should be the final output attachment(s) with sample count = 1 (probably from ISwapchain::GetImageView())
		//- _depthStencilAttachment must have a sample count equal to that of the _multisampleColourAttachment(s)
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthStencilAttachment) = 0;

		//Blit _srcTexture to _dstTexture with linear downsampling
		//Requires:
		//- _srcTexture->GetSize() >= _dstTexture->GetSize()
		//- _srcTexture is in RESOURCE_STATE::COPY_SOURCE state
		//- _dstTexture is in RESOURCE_STATE::COPY_DEST state
		virtual void BlitTexture(ITexture* _srcTexture, TEXTURE_ASPECT _srcAspect, ITexture* _dstTexture, TEXTURE_ASPECT _dstAspect) = 0;
		
		virtual void EndRendering() = 0;

		virtual void BindVertexBuffers(std::uint32_t _firstBinding, std::uint32_t _bindingCount, IBuffer* _buffers, std::size_t* _strides) = 0;
		virtual void BindIndexBuffer(IBuffer* _buffer, DATA_FORMAT _format) = 0;
		virtual void BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint) = 0;
		virtual void BindRootSignature(IRootSignature* _rootSignature, PIPELINE_BIND_POINT _bindPoint) = 0;
		virtual void PushConstants(IRootSignature* _rootSignature, void* _data) = 0;
		virtual void SetViewport(glm::vec2 _pos, glm::vec2 _extent) = 0;
		virtual void SetScissor(glm::ivec2 _pos, glm::ivec2 _extent) = 0;
		virtual void DrawIndexed(std::uint32_t _indexCount, std::uint32_t _instanceCount, std::uint32_t _firstIndex, std::uint32_t _firstInstance) = 0;
		
		virtual void CopyBufferToBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer, std::size_t _srcOffset, std::size_t _dstOffset, std::size_t _size) = 0;
		//_srcOffset in bytes, _dstOffset in texels for XYZ, _dstExtent in texels for XYZ
		virtual void CopyBufferToTexture(IBuffer* _srcBuffer, ITexture* _dstTexture, std::size_t _srcOffset, glm::ivec3 _dstOffset, glm::ivec3 _dstExtent) = 0;


	protected:
		explicit ICommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
		: m_logger(_logger), m_device(_device), m_pool(_pool), m_level(_desc.level) {}

		//NVI implementations
		virtual void TransitionBarrierImpl(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) = 0;
		virtual void TransitionBarrierImpl(IBuffer* _buffer, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) = 0;


		//Dependency injections
		ILogger& m_logger;
		IDevice& m_device;
		ICommandPool& m_pool;

		COMMAND_BUFFER_LEVEL m_level;
	};
	
}