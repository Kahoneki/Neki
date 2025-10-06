#include "D3D12Buffer.h"
#include <utility>
#include <Core/Utils/EnumUtils.h>
#ifdef ERROR
	#undef ERROR //conflicts with LOGGER_CHANNEL::ERROR
#endif

namespace NK
{

	D3D12Buffer::D3D12Buffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc)
	: IBuffer(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Initialising D3D12Buffer\n");


		//Define heap props
		D3D12_HEAP_PROPERTIES heapProps{};
		switch (m_memType)
		{
		case MEMORY_TYPE::HOST:		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;	break;
		case MEMORY_TYPE::DEVICE:	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;	break;
		}
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;


		//Create buffer
		m_resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		m_resourceDesc.Alignment = 0;
		m_resourceDesc.Width = m_size;
		m_resourceDesc.Height = 1;
		m_resourceDesc.DepthOrArraySize = 1;
		m_resourceDesc.MipLevels = 1;
		m_resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		m_resourceDesc.SampleDesc.Count = 1;
		m_resourceDesc.SampleDesc.Quality = 0;
		m_resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		m_resourceDesc.Flags = GetCreationFlags();

		HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &m_resourceDesc, GetInitialState(), nullptr, IID_PPV_ARGS(&m_buffer)) };
	

		m_logger.Unindent();
	}
	
	
	
	D3D12Buffer::~D3D12Buffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down D3D12Buffer\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	void* D3D12Buffer::Map()
	{
		void* data;
		m_buffer->Map(0, nullptr, &data);
		return data;
	}
	
	
	
	void D3D12Buffer::Unmap()
	{
		m_buffer->Unmap(0, nullptr);
	}



	D3D12_RESOURCE_FLAGS D3D12Buffer::GetCreationFlags() const
	{
		D3D12_RESOURCE_FLAGS d3d12Flags{ D3D12_RESOURCE_FLAG_NONE };

		if ((m_usage & BUFFER_USAGE_FLAGS::STORAGE_BUFFER_BIT) != BUFFER_USAGE_FLAGS::NONE) { d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; }
	
		return d3d12Flags;
	}



	D3D12_RESOURCE_STATES D3D12Buffer::GetInitialState() const
	{
		if (m_memType == MEMORY_TYPE::HOST)
		{
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		}
	
		return D3D12_RESOURCE_STATE_COMMON;
	}

}