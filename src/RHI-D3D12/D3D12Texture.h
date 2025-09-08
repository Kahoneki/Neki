#pragma once
#include "RHI/ITexture.h"
#include "D3D12Device.h"

namespace NK
{
	class D3D12Texture final : public ITexture
	{
	public:
		explicit D3D12Texture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc);
		//For wrapping an existing ID3D12Resource
		explicit D3D12Texture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, ID3D12Resource* _resource);

		virtual ~D3D12Texture() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] static DXGI_FORMAT GetDXGIFormat(TEXTURE_FORMAT _format);
		[[nodiscard]] ID3D12Resource* GetResource() const { return m_texture.Get(); }


	private:
		[[nodiscard]] D3D12_RESOURCE_FLAGS GetCreationFlags() const;
		[[nodiscard]] D3D12_RESOURCE_STATES GetInitialState() const;


		Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	};
}
