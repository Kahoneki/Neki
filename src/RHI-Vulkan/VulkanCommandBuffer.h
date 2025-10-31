#pragma once

#include <RHI/ICommandBuffer.h>


namespace NK
{
	
	class VulkanCommandBuffer final : public ICommandBuffer
	{
	public:
		explicit VulkanCommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc);
		virtual ~VulkanCommandBuffer() override;

		virtual void Reset() override;

		virtual void Begin() override;
		virtual void SetBlendConstants(const float _blendConstants[4]) override;
		virtual void End() override;

		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthAttachment, ITextureView* _stencilAttachment) override;
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _multisampleColourAttachments, ITextureView* _outputColourAttachments, ITextureView* _depthStencilAttachment) override;
		virtual void BlitTexture(ITexture* _srcTexture, TEXTURE_ASPECT _srcAspect, ITexture* _dstTexture, TEXTURE_ASPECT _dstAspect) override;
		virtual void EndRendering(std::size_t _numColourAttachments, ITexture* _multisampleColourAttachments, ITexture* _outputColourAttachments) override;

		virtual void BindVertexBuffers(std::uint32_t _firstBinding, std::uint32_t _bindingCount, IBuffer* _buffers, std::size_t* _strides) override;
		virtual void BindIndexBuffer(IBuffer* _buffer, DATA_FORMAT _format) override;
		virtual void BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint) override;
		virtual void BindRootSignature(IRootSignature* _rootSignature, PIPELINE_BIND_POINT _bindPoint) override;
		virtual void PushConstants(IRootSignature* _rootSignature, void* _data) override;
		virtual void SetViewport(glm::vec2 _pos, glm::vec2 _extent) override;
		virtual void SetScissor(glm::ivec2 _pos, glm::ivec2 _extent) override;
		virtual void DrawIndexed(std::uint32_t _indexCount, std::uint32_t _instanceCount, std::uint32_t _firstIndex, std::uint32_t _firstInstance) override;

		virtual void CopyBufferToBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer, std::size_t _srcOffset, std::size_t _dstOffset, std::size_t _size) override;
		virtual void CopyBufferToTexture(IBuffer* _srcBuffer, ITexture* _dstTexture, std::size_t _srcOffset, glm::ivec3 _dstOffset, glm::ivec3 _dstExtent) override;
		
		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkCommandBuffer GetBuffer() const { return m_buffer; }


	protected:
		//NVI implementations
		virtual void TransitionBarrierImpl(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) override;
		virtual void TransitionBarrierImpl(IBuffer* _buffer, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) override;


	private:
		VkCommandBuffer m_buffer{ VK_NULL_HANDLE };
	};
	
}
