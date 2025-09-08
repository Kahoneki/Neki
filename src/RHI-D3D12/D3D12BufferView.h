#pragma once
#include <RHI/IBufferView.h>
#include "D3D12Device.h"


namespace NK
{

	class D3D12BufferView final : public IBufferView
	{
	public:
		explicit D3D12BufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize);
		~D3D12BufferView();
	};
	
}