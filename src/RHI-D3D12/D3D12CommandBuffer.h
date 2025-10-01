#pragma once
#include "RHI/ICommandBuffer.h"
#include "D3D12Device.h"

namespace NK
{

	class D3D12CommandBuffer final : public ICommandBuffer
	{
	public:
		explicit D3D12CommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc);
		virtual ~D3D12CommandBuffer() override;

		//todo: add command buffer methods here
		virtual void Reset() override;

		virtual void Begin() override;
		virtual void SetBlendConstants(const float _blendConstants[4]) override;
		virtual void End() override;

		virtual void TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) override;
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthStencilAttachment) override;
		virtual void EndRendering() override;

		virtual void BindVertexBuffers(std::uint32_t _firstBinding, std::uint32_t _bindingCount, IBuffer* _buffers, std::size_t* _strides) override;
		virtual void BindIndexBuffer(IBuffer* _buffer, DATA_FORMAT _format) override;
		virtual void BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint) override;
		virtual void SetViewport(glm::vec2 _pos, glm::vec2 _extent) override;
		virtual void SetScissor(glm::ivec2 _pos, glm::ivec2 _extent) override;
		virtual void DrawIndexed(std::uint32_t _indexCount, std::uint32_t _instanceCount, std::uint32_t _firstIndex, std::uint32_t _firstInstance) override;

		virtual void CopyBuffer(IBuffer* _srcBuffer, IBuffer* _dstBuffer) override;
		virtual void UploadDataToDeviceBuffer(void* data, std::size_t size, IBuffer* _dstBuffer) override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12GraphicsCommandList* GetCommandList() const { return m_buffer.Get(); }


	private:
		[[nodiscard]] D3D12_RESOURCE_STATES GetD3D12ResourceState(RESOURCE_STATE _state) const;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_buffer;
	};
}
