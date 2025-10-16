#pragma once

#include "D3D12Device.h"

#include <RHI/IBuffer.h>


namespace NK
{
	
	class D3D12Buffer final : public IBuffer
	{
	public:
		explicit D3D12Buffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc);
		virtual ~D3D12Buffer() override;

		virtual void* Map() override;
		virtual void Unmap() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12Resource* GetBuffer() const { return m_buffer.Get(); }
		[[nodiscard]] inline D3D12_RESOURCE_DESC GetResourceDesc() const { return m_resourceDesc; }


	private:
		[[nodiscard]] D3D12_RESOURCE_FLAGS GetCreationFlags() const;
		[[nodiscard]] D3D12_RESOURCE_STATES GetInitialState() const;


		Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
		D3D12_RESOURCE_DESC m_resourceDesc;
	};

}
