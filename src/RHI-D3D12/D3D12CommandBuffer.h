#pragma once
#include "D3D12Device.h"
#include "RHI/ICommandBuffer.h"

namespace NK
{
	class D3D12CommandBuffer final : public ICommandBuffer
	{
	public:
		explicit D3D12CommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc);
		virtual ~D3D12CommandBuffer() override;

		//todo: add command buffer methods here
		virtual void Reset() override;


	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_buffer;
	};
}
