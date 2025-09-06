#pragma once
#include <RHI/ITextureView.h>
#include "D3D12Device.h"

namespace NK
{

	class D3D12TextureView final : public ITextureView
	{
	public:
		//Use this constructor if you want a free list allocator to be used for allocation (will be the case for shader-accessible view types)
		explicit D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, FreeListAllocator* _freeListAllocator);

		//Use this constructor to bypass the free list allocation if you have your own index you'd like to use (will be the case for non-shader accessible view types)
		explicit D3D12TextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize, std::uint32_t _resourceIndex);

		virtual ~D3D12TextureView() override;
	};

}