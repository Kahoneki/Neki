#include "D3D12RootSignature.h"
#include <stdexcept>
#include <array>
#include <Core/Utils/FormatUtils.h>
#ifdef ERROR
	#undef ERROR //Conflicts with LOGGER_CHANNEL::ERROR
#endif

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
		std::array<D3D12_ROOT_PARAMETER1, 3> rootParams;


		//Root parameter 0: resources (cbvs, srvs, and uavs)
		D3D12_DESCRIPTOR_RANGE1 resourceRange{};
		resourceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //ew
		resourceRange.NumDescriptors = -1; //unbounded - go until end of heap
		resourceRange.BaseShaderRegister = 0; //resources start at t0
		resourceRange.RegisterSpace = 0; //resource range in register space 0
		resourceRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		resourceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &resourceRange;


		//Root parameter 1: samplers
		D3D12_DESCRIPTOR_RANGE1 samplerRange{};
		samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		samplerRange.NumDescriptors = -1; //unbounded - go until end of heap
		samplerRange.BaseShaderRegister = 0; //resources start at s0
		samplerRange.RegisterSpace = 0; //resource range in register space 0
		samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; //hint that descriptors can change
		samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &samplerRange;


		//Root parameter 2: root constants
		m_actualNum32BitPushConstantValues = m_providedNum32BitPushConstantValues; //Irrelevant for dx12, just for parity with vulkan
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[2].Constants.Num32BitValues = m_actualNum32BitPushConstantValues;
		rootParams[2].Constants.ShaderRegister = 0;
		rootParams[2].Constants.RegisterSpace = 0;


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
		m_resourceDescriptorTable.rootParamIndex = 0;
		m_resourceDescriptorTable.gpuHandle = m_resourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_samplerDescriptorTable.rootParamIndex = 1;
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