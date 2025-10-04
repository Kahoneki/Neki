#pragma once
#include <RHI/ISampler.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12Sampler final : public ISampler
	{
	public:
		explicit D3D12Sampler(ILogger& _logger, IAllocator& _allocator, FreeListAllocator& _freeListAllocator, IDevice& _device, const SamplerDesc& _desc);
		virtual ~D3D12Sampler() override;

		[[nodiscard]] static D3D12_FILTER GetD3D12Filter(FILTER_MODE _minFilterMode, FILTER_MODE _magFilterMode, FILTER_MODE _mipFilterMode);
		[[nodiscard]] static D3D12_TEXTURE_ADDRESS_MODE GetD3D12AddressMode(ADDRESS_MODE _addressMode);
	};
}