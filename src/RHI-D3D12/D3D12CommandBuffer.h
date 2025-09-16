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

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12GraphicsCommandList* GetCommandList() const { return m_buffer.Get(); }


	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_buffer;
	};
}
