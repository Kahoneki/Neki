#include "D3D12BufferView.h"
#include "D3D12Buffer.h"
#include <Core/Utils/FormatUtils.h>
#include <stdexcept>
#ifdef ERROR
	#undef ERROR
#endif

namespace NK
{

	D3D12BufferView::D3D12BufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc, ID3D12DescriptorHeap* _descriptorHeap, UINT _descriptorSize)
	: IBufferView(_logger, _allocator, _device, _buffer, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER_VIEW, "Creating buffer view\n");

		
		//Get resource index from free list
		m_resourceIndex = m_allocator.Allocate();
		if (m_resourceIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER_VIEW, "Resource index allocation failed - max bindless resource count (" + std::to_string(MAX_BINDLESS_RESOURCES) + ") reached.\n");
			throw std::runtime_error("");
		}


		//Get underlying D3D12Buffer
		const D3D12Buffer* d3d12Buffer{ dynamic_cast<const D3D12Buffer*>(_buffer) };
		if (!d3d12Buffer)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER_VIEW, "Dynamic cast to D3D12Buffer object expected to pass but failed\n");
			throw std::runtime_error("");
		}


		//Calculate memory address of descriptor slot
		D3D12_CPU_DESCRIPTOR_HANDLE addr{ _descriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		addr.ptr += m_resourceIndex * _descriptorSize;

		
		//Create appropriate view based on provided type
		switch (_desc.type)
		{
		case BUFFER_VIEW_TYPE::UNIFORM:
		{
			//d3d12 requires cbv size is a multiple of 256 bytes
			if (_desc.size % 256 != 0)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "_desc.type = BUFFER_VIEW_TYPE::CONSTANT but _desc.size (" + FormatUtils::GetSizeString(_desc.size) + ") is not a multiple of 256 bytes as required by d3d12 for cbvs\n");
				throw std::runtime_error("");
			}

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = d3d12Buffer->GetBuffer()->GetGPUVirtualAddress() + _desc.offset;
			cbvDesc.SizeInBytes = static_cast<UINT>(_desc.size);
			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateConstantBufferView(&cbvDesc, addr);
			break;
		}
		case BUFFER_VIEW_TYPE::STORAGE_READ_ONLY:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = static_cast<UINT>(_desc.offset / 4); //offset is measured in elements (4 bytes for r32 format)
			srvDesc.Buffer.NumElements = static_cast<UINT>(_desc.size / 4);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW; //ByteAddressBuffer instead of StructuredBuffer

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateShaderResourceView(d3d12Buffer->GetBuffer(), &srvDesc, addr);
			break;
		}
		case BUFFER_VIEW_TYPE::STORAGE_READ_WRITE:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = static_cast<UINT>(_desc.offset / 4); //offset is measured in elements (4 bytes for r32 format)
			uavDesc.Buffer.NumElements = static_cast<UINT>(_desc.size / 4);
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

			dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateUnorderedAccessView(d3d12Buffer->GetBuffer(), nullptr, &uavDesc, addr);
			break;
		}
		}
		

		m_logger.Unindent();
	}



	D3D12BufferView::~D3D12BufferView()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER_VIEW, "Shutting Down D3D12BufferView\n");

		if (m_resourceIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_allocator.Free(m_resourceIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER_VIEW, "Resource Index Freed\n");
			m_resourceIndex = FreeListAllocator::INVALID_INDEX;
		}
		
		m_logger.Unindent();
	}
	
}