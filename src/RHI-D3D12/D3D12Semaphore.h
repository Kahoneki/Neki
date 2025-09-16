#pragma once
#include <RHI/ISemaphore.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12Semaphore final : public ISemaphore
	{
	public:
		explicit D3D12Semaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device);
		virtual ~D3D12Semaphore() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12Fence* GetFence() const { return m_fence.Get(); }
		[[nodiscard]] inline std::uint64_t GetFenceValue() const { return m_fenceValue; }
		inline void IncrementFenceValue() { ++m_fenceValue; }
		

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

		//Current value on the fence's timeline
		//This is the value the producer queue will signal and that the consumer will wait for
		std::uint64_t m_fenceValue{ 0 };
	};
}