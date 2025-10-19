#include "D3D12RootSignature.h"

#include <Core/Utils/FormatUtils.h>

#include <array>
#include <stdexcept>


namespace NK
{

	D3D12RootSignature::D3D12RootSignature(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const RootSignatureDesc& _desc)
	: IRootSignature(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ROOT_SIGNATURE, "Initialising D3D12RootSignature\n");

		
		m_resourceDescriptorHeap = dynamic_cast<D3D12Device&>(m_device).GetResourceDescriptorHeap();
		m_samplerDescriptorHeap = dynamic_cast<D3D12Device&>(m_device).GetSamplerDescriptorHeap();
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::ROOT_SIGNATURE, "Global descriptor heaps pulled from D3D12Device\n");

		
		//Create the root signature
		std::array<D3D12_ROOT_PARAMETER1, 6> rootParams;


		//Root parameters 0, 1, 2, 3: resources (cbvs, srvs, and uavs)
		D3D12_DESCRIPTOR_RANGE1 cbvResourceRange{};
		cbvResourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvResourceRange.NumDescriptors = -1; //unbounded - go until end of heap
		cbvResourceRange.BaseShaderRegister = 0; //resources start at b0
		cbvResourceRange.RegisterSpace = 0; //resource range in register space 0
		cbvResourceRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //DATA_STATIC flag on an unbounded range gets ignored and treated as volatile
		cbvResourceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//Texture2D
		D3D12_DESCRIPTOR_RANGE1 tex2DSRVResourceRange{};
		tex2DSRVResourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		tex2DSRVResourceRange.NumDescriptors = -1; //unbounded - go until end of heap
		tex2DSRVResourceRange.BaseShaderRegister = 0; //resources start at t0
		tex2DSRVResourceRange.RegisterSpace = 0; //resource range in register space 0
		tex2DSRVResourceRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		tex2DSRVResourceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//TextureCube
		D3D12_DESCRIPTOR_RANGE1 texCubeSRVResourceRange{};
		texCubeSRVResourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		texCubeSRVResourceRange.NumDescriptors = -1; //unbounded - go until end of heap
		texCubeSRVResourceRange.BaseShaderRegister = 0; //resources start at t0
		texCubeSRVResourceRange.RegisterSpace = 1; //resource range in register space 0
		texCubeSRVResourceRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		texCubeSRVResourceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE1 uavResourceRange{};
		uavResourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavResourceRange.NumDescriptors = -1; //unbounded - go until end of heap
		uavResourceRange.BaseShaderRegister = 0; //resources start at u0
		uavResourceRange.RegisterSpace = 0; //resource range in register space 0
		uavResourceRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		uavResourceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvResourceRange;

		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &tex2DSRVResourceRange;

		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &texCubeSRVResourceRange;

		rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[3].DescriptorTable.pDescriptorRanges = &uavResourceRange;


		//Root parameter 3: samplers
		D3D12_DESCRIPTOR_RANGE1 samplerRange{};
		samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		samplerRange.NumDescriptors = -1; //unbounded - go until end of heap
		samplerRange.BaseShaderRegister = 0; //resources start at s0
		samplerRange.RegisterSpace = 0; //resource range in register space 0
		samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[4].DescriptorTable.pDescriptorRanges = &samplerRange;


		//Root parameter 4: root constants
		m_actualNum32BitPushConstantValues = m_providedNum32BitPushConstantValues; //Irrelevant for dx12, just for parity with vulkan
		rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[5].Constants.Num32BitValues = m_actualNum32BitPushConstantValues;
		rootParams[5].Constants.ShaderRegister = 0;
		rootParams[5].Constants.RegisterSpace = 1; //So as to not conflict with buffers in resource descriptor heap


		//Serialise root signature
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParams.size());
		rootSigDesc.Desc_1_1.pParameters = rootParams.data();
		rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
		rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
		rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ID3DBlob* serialisedRootSig;
		ID3DBlob* errorStr;
		HRESULT hr{ D3D12SerializeVersionedRootSignature(&rootSigDesc, &serialisedRootSig, &errorStr) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::ROOT_SIGNATURE, "Root Signature serialisation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::ROOT_SIGNATURE, "Root Signature serialisation unsuccessful - error blob: ");
			m_logger.IndentRawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::ROOT_SIGNATURE, static_cast<const char*>(errorStr->GetBufferPointer()) + std::string("\n"));
			throw std::runtime_error("");
		}


		//Create root signature
		hr = dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateRootSignature(0, serialisedRootSig->GetBufferPointer(), serialisedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSig));
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::ROOT_SIGNATURE, "Root Signature creation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::ROOT_SIGNATURE, "Root Signature creation unsuccessful\n");
			throw std::runtime_error("");
		}


		//Populate descriptor table structs
		m_cbvDescriptorTable.rootParamIndex = 0;
		m_cbvDescriptorTable.gpuHandle = m_resourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_tex2DSRVDescriptorTable.rootParamIndex = 1;
		m_tex2DSRVDescriptorTable.gpuHandle = m_resourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_texCubeSRVDescriptorTable.rootParamIndex = 2;
		m_texCubeSRVDescriptorTable.gpuHandle = m_resourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_uavDescriptorTable.rootParamIndex = 3;
		m_uavDescriptorTable.gpuHandle = m_resourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_samplerDescriptorTable.rootParamIndex = 4;
		m_samplerDescriptorTable.gpuHandle = m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();


		m_logger.Unindent();
	}



	D3D12RootSignature::~D3D12RootSignature()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ROOT_SIGNATURE, "Shutting Down D3D12RootSignature\n");

		//m_resourceDescriptorHeap is owned by the D3D12Device
		//m_samplerDescriptorHeap is owned by the D3D12Device
		//ComPtrs are released automatically
		
		m_logger.Unindent();
	}
	
}