#pragma once
#include <RHI/IBufferView.h>


namespace NK
{

	class D3D12BufferView final : public IBufferView
	{
	public:
		explicit D3D12BufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc);
		~D3D12BufferView();
	};
	
}