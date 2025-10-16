#include "D3D12Sampler.h"

#include "D3D12Pipeline.h"

#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


namespace NK
{

	D3D12Sampler::D3D12Sampler(ILogger& _logger, IAllocator& _allocator, FreeListAllocator& _freeListAllocator, IDevice& _device, const SamplerDesc& _desc)
	: ISampler(_logger, _allocator, _freeListAllocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SAMPLER, "Initialising D3D12Sampler\n");

		//Get resource index from free list
		m_samplerIndex = m_freeListAllocator.Allocate();
		if (m_samplerIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SAMPLER, "Resource index allocation failed - max bindless sampler count (" + std::to_string(MAX_BINDLESS_SAMPLERS) + ") reached.\n");
			throw std::runtime_error("");
		}

		D3D12Device& d3d12Device{ dynamic_cast<D3D12Device&>(_device) };

		//Get CPU descriptor handle
		D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle{ d3d12Device.GetSamplerDescriptorHeap()->GetCPUDescriptorHandleForHeapStart() };
		descriptorHandle.ptr += m_samplerIndex * d3d12Device.GetSamplerDescriptorSize();

		//Create the sampler
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = GetD3D12Filter(_desc.minFilter, _desc.magFilter, _desc.mipmapFilter);
		samplerDesc.AddressU = GetD3D12AddressMode(_desc.addressModeU);
		samplerDesc.AddressW = GetD3D12AddressMode(_desc.addressModeV);
		samplerDesc.AddressV = GetD3D12AddressMode(_desc.addressModeW);
		samplerDesc.MipLODBias = _desc.mipLODBias;
		samplerDesc.MaxAnisotropy = _desc.maxAnisotropy;
		samplerDesc.ComparisonFunc = D3D12Pipeline::GetD3D12ComparisonFunc(_desc.compareOp);
		samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = _desc.minLOD;
		samplerDesc.MaxLOD = _desc.maxLOD;
		dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateSampler(&samplerDesc, descriptorHandle);


		m_logger.Unindent();
	}

	
	
	D3D12_FILTER D3D12Sampler::GetD3D12Filter(FILTER_MODE _minFilterMode, FILTER_MODE _magFilterMode, FILTER_MODE _mipFilterMode)
	{
		//ANISOTROPIC and COMPARISON filters ignored for this implementation

		if (_minFilterMode == FILTER_MODE::NEAREST)
		{
			if (_magFilterMode == FILTER_MODE::NEAREST)
			{
				if (_mipFilterMode == FILTER_MODE::NEAREST) return D3D12_FILTER_MIN_MAG_MIP_POINT;
				if (_mipFilterMode == FILTER_MODE::LINEAR)  return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			}
			else if (_magFilterMode == FILTER_MODE::LINEAR)
			{
				if (_mipFilterMode == FILTER_MODE::NEAREST) return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
				if (_mipFilterMode == FILTER_MODE::LINEAR)  return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
		}
		else if (_minFilterMode == FILTER_MODE::LINEAR)
		{
			if (_magFilterMode == FILTER_MODE::NEAREST)
			{
				if (_mipFilterMode == FILTER_MODE::NEAREST) return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
				if (_mipFilterMode == FILTER_MODE::LINEAR)  return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			}
			else if (_magFilterMode == FILTER_MODE::LINEAR)
			{
				if (_mipFilterMode == FILTER_MODE::NEAREST) return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
				if (_mipFilterMode == FILTER_MODE::LINEAR)  return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			}
		}

		throw std::runtime_error("Default case reached for D3D12Sampler::GetD3D12Filter() - unsupported filter combination");
	}



	D3D12_TEXTURE_ADDRESS_MODE D3D12Sampler::GetD3D12AddressMode(ADDRESS_MODE _addressMode)
	{
		switch (_addressMode)
		{
		case ADDRESS_MODE::REPEAT:					return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case ADDRESS_MODE::MIRRORED_REPEAT:			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case ADDRESS_MODE::CLAMP_TO_EDGE:			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case ADDRESS_MODE::CLAMP_TO_BORDER:			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case ADDRESS_MODE::MIRROR_CLAMP_TO_EDGE:	return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Sampler::GetD3D12AddressMode() - address mode = " + std::to_string(std::to_underlying(_addressMode)));
		}
		}
	}



	D3D12Sampler::~D3D12Sampler()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SAMPLER, "Shutting Down D3D12Sampler\n");

		
		if (m_samplerIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_freeListAllocator.Free(m_samplerIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SAMPLER, "Sampler Index Freed\n");
			m_samplerIndex = FreeListAllocator::INVALID_INDEX;
		}

		
		m_logger.Unindent();
	}
	
}