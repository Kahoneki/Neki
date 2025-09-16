#pragma once
#include <RHI/IFence.h>
#include "D3D12Device.h"

namespace NK
{
	//State of the fence - to mimic Vulkan's stricter fence usage rules
	enum class FENCE_STATE
	{
		UNSIGNALLED, //Ready to be used in a queue submit
		IN_FLIGHT, //In use by the GPU
		SIGNALLED, //The GPU has finished its execution
	};


	class D3D12Fence final : public IFence
	{
	public:
		explicit D3D12Fence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc);
		virtual ~D3D12Fence() override;

		virtual void Wait() override;
		virtual void Reset() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12Fence* GetFence() const { return m_fence.Get(); }
		[[nodiscard]] inline std::uint64_t GetFenceValue() const { return m_fenceValue; }
		inline void IncrementFenceValue() { ++m_fenceValue; }
		[[nodiscard]] inline FENCE_STATE GetState() const { return m_state; };
		inline void SetState(FENCE_STATE _state) { m_state = _state; }


	private:
		FENCE_STATE m_state;

		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fenceEvent;
		//Value the fence will have after the next signal from the GPU
		//This is the internal fence value the producer queue will signal after Submit() and that the consumer will wait for when Wait() is called
		std::uint64_t m_fenceValue{ 0 };
	};
}