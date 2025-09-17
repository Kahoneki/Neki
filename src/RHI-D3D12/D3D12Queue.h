#pragma once
#include <RHI/IQueue.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12Queue final : public IQueue
	{	
	public:
		explicit D3D12Queue(ILogger& _logger, IDevice& _device, const QueueDesc& _desc);
		virtual ~D3D12Queue() override;

		virtual void Submit(ICommandBuffer* _cmdBuffer, ISemaphore* _waitSemaphore, ISemaphore* _signalSemaphore, IFence* _signalFence) override;
		virtual void WaitIdle() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }


	private:
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
	};
}