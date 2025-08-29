#pragma once
#include "RHI/ITexture.h"
#include "D3D12Device.h"

namespace NK
{
	class D3D12Texture final : public ITexture
	{
	public:
		explicit D3D12Texture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc);
		virtual ~D3D12Texture() override;


	private:
		[[nodiscard]] D3D12_RESOURCE_FLAGS GetCreationFlags() const;
		[[nodiscard]] DXGI_FORMAT GetDXGIFormat() const;
		[[nodiscard]] D3D12_RESOURCE_STATES GetInitialState() const;


		Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	};
}
