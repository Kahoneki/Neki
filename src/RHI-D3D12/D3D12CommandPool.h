#pragma once
#include <RHI/ICommandPool.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12Device;

	//ID3D12CommandAllocator
	class D3D12CommandPool final : public ICommandPool
	{
	public:
		explicit D3D12CommandPool(ILogger& _logger, IAllocator& _allocator, D3D12Device& _device, const CommandPoolDesc& _desc);
		virtual ~D3D12CommandPool() override;

		//ICommandPool interface implementation
		[[nodiscard]] virtual UniquePtr<ICommandBuffer> AllocateCommandBuffer(const CommandBufferDesc& _desc) override;
		virtual void Reset(COMMAND_POOL_RESET_FLAGS _type) override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12CommandAllocator* GetCommandPool() const { return m_pool.Get(); }


	private:
		[[nodiscard]] std::string GetPoolTypeString() const;

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pool;
	};
}