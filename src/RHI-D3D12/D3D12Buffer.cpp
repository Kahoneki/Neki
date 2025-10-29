#include "D3D12Buffer.h"

#include <Core/Utils/EnumUtils.h>
#include <Core/Utils/FormatUtils.h>


namespace NK
{

	D3D12Buffer::D3D12Buffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc)
	: IBuffer(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Initialising D3D12Buffer\n");


		//D3D12 requires CBVs are multiples of 256B. For safety, if the usage flags contains uniform buffer bit, round up to nearest multiple of 256B
		if (EnumUtils::Contains(m_usage, BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT))
		{
			if (m_size % 256 != 0)
			{
				m_size = (m_size + 255) & ~255; //Round up to nearest multiple of 256
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::BUFFER, "_desc.usage contained NK::BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT, but _desc.size (" + FormatUtils::GetSizeString(_desc.size) + ") is not a multiple of 256B as required for CBVs in D3D12. For parity and safety, all uniform buffers are required to be multiples of 256B. Rounding up to nearest multiple of 256B (" + FormatUtils::GetSizeString(m_size) + ")\n");
			}
		}


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

		D3D12MA::ALLOCATION_DESC allocDesc{};
		switch (m_memType)
		{
		case MEMORY_TYPE::HOST:		allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;	break;
		case MEMORY_TYPE::DEVICE:	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;	break;
		}
		//todo: D3D12MA::ALLOCATION_FLAGS has some really cool stuff i wanna play around with

		HRESULT hr{ dynamic_cast<D3D12Device&>(m_device).GetD3D12MAAllocator()->CreateResource(&allocDesc, &m_resourceDesc, D3D12_RESOURCE_STATE_COMMON, NULL, &m_allocation, IID_PPV_ARGS(&m_buffer)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "ID3D12Resource initialisation and allocation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "ID3D12Resource initialisation or allocation unsuccessful. hr = " + std::to_string(hr) + '\n');
			throw std::runtime_error("");
		}


		if (m_memType == MEMORY_TYPE::HOST)
		{
			hr = m_buffer->Map(0, nullptr, &m_map);
			if (SUCCEEDED(hr))
			{
				m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "ID3D12Resource mapping successful\n");
			}
			else
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "ID3D12Resource mapping unsuccessful. hr = " + std::to_string(hr) + '\n');
				throw std::runtime_error("");
			}
		}

		m_logger.Unindent();
	}
	
	
	
	D3D12Buffer::~D3D12Buffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down D3D12Buffer\n");

		//ComPtrs are released automatically

		if (m_map != nullptr)
		{
			m_buffer->Unmap(0, nullptr);
			m_map = nullptr;
		}

		m_logger.Unindent();
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