#include "D3D12Device.h"
#include "D3D12Device.h"
#include "Core/Memory/Allocation.h"
#include "Core/Utils/FormatUtils.h"
#include "D3D12CommandPool.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12Surface.h"
#include <stdexcept>
#include <array>
#ifdef ERROR
	#undef ERROR //conflicts with LOGGER_CHANNEL::ERROR
#endif

namespace NK
{

	NK::D3D12Device::D3D12Device(ILogger& _logger, IAllocator& _allocator)
		: IDevice(_logger, _allocator)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Initialising D3D12Device\n");

		if (m_enableDebugLayer) { EnableDebugLayer(); }
		CreateFactory();
		SelectAdapter();
		CreateDevice();
		CreateCommandQueues();
		CreateDescriptorHeaps();
		CreateRootSignature();

		m_logger.Unindent();
	}



	D3D12Device::~D3D12Device()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Shutting Down D3D12Device\n");
		
		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	UniquePtr<IBuffer> D3D12Device::CreateBuffer(const BufferDesc& _desc)
	{
		return UniquePtr<IBuffer>(NK_NEW(D3D12Buffer, m_logger, m_allocator, *this, _desc));
	}



	ResourceIndex D3D12Device::CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Creating buffer view\n");

		//Get resource index from free list
		const ResourceIndex index{ m_resourceIndexAllocator->Allocate() };
		if (index == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Resource index allocation failed - max bindless resource count (" + std::to_string(MAX_BINDLESS_RESOURCES) + ") reached.\n");
			throw std::runtime_error("");
		}

		//Get underlying D3D12Buffer
		const D3D12Buffer* d3d12Buffer{ dynamic_cast<const D3D12Buffer*>(_buffer) };
		if (!d3d12Buffer)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Dynamic cast to D3D12Buffer object expected to pass but failed\n");
			throw std::runtime_error("");
		}

		//Calculate memory address of descriptor slot
		D3D12_CPU_DESCRIPTOR_HANDLE addr{ m_resourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		addr.ptr += index * m_resourceDescriptorSize;

		//Create appropriate view based on provided type
		switch (_desc.type)
		{
		case BUFFER_VIEW_TYPE::CONSTANT:
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
			m_device->CreateConstantBufferView(&cbvDesc, addr);
			break;
		}
		case BUFFER_VIEW_TYPE::SHADER_RESOURCE:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = static_cast<UINT>(_desc.offset / 4); //offset is measured in elements (4 bytes for r32 format)
			srvDesc.Buffer.NumElements = static_cast<UINT>(_desc.size / 4);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

			m_device->CreateShaderResourceView(d3d12Buffer->GetBuffer(), &srvDesc, addr);
			break;
		}
		case BUFFER_VIEW_TYPE::UNORDERED_ACCESS:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = static_cast<UINT>(_desc.offset / 4); //offset is measured in elements (4 bytes for r32 format)
			uavDesc.Buffer.NumElements = static_cast<UINT>(_desc.size / 4);
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			
			m_device->CreateUnorderedAccessView(d3d12Buffer->GetBuffer(), nullptr, &uavDesc, addr);
			break;
		}
		}

		m_logger.Unindent();
	
		return index;
	}



	UniquePtr<ITexture> D3D12Device::CreateTexture(const TextureDesc& _desc)
	{
		return UniquePtr<ITexture>(NK_NEW(D3D12Texture, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ICommandPool> D3D12Device::CreateCommandPool(const CommandPoolDesc& _desc)
	{
		return UniquePtr<ICommandPool>(NK_NEW(D3D12CommandPool, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISurface> D3D12Device::CreateSurface(const SurfaceDesc& _desc)
	{
		return UniquePtr<ISurface>(NK_NEW(D3D12Surface, m_logger, m_allocator, *this, _desc));
	}



	void D3D12Device::EnableDebugLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Enabling debug layer\n");

		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		const HRESULT result{ D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) };
		if (SUCCEEDED(result))
		{
			debugController->EnableDebugLayer();
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Debug layer successfully enabled\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to enable debug layer\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	void D3D12Device::CreateFactory()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating factory\n");

		UINT createFactoryFlags{ 0 };
		if (m_enableDebugLayer)
		{
			createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		const HRESULT result{ CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_factory)) };
		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Factory successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create factory\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	void D3D12Device::SelectAdapter()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Selecting physical adapter\n");

		//Select adapter based on dedicated memory size (for now)
		//todo: update this^ logic to be more in line with VulkanDevice::SelectPhysicalDevice()
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		SIZE_T maxDedicatedVideoMemory{ 0 };

		for (UINT i{ 0 }; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			//Skip software adapters - discrete gpu supremacy
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			//Check if the adapter can be used to create a d3d12 device
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_featureLevel, __uuidof(ID3D12Device), nullptr)))
			{
				if (desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
				{
					maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
					m_adapter = adapter;
				}
			}
		}

		if (m_adapter.Get() == nullptr)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "No suitable D3D12-FL12.0 discrete-adapter found.\n");
			throw std::runtime_error("");
		}

		DXGI_ADAPTER_DESC1 desc;
		m_adapter->GetDesc1(&desc);
		std::wstring wstr{ desc.Description };
		std::string name{ wstr.begin(), wstr.end() };
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Selected adapter: " + name + '\n');

		m_logger.Unindent();
	}



	void D3D12Device::CreateDevice()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating device\n");

		const HRESULT hr{ D3D12CreateDevice(m_adapter.Get(), m_featureLevel, IID_PPV_ARGS(&m_device)) };

		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Device successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create device\n");
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	void D3D12Device::CreateCommandQueues()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating command queues\n");

		//Graphics (Direct) Queue
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		HRESULT hr{ m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsQueue)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Graphics Command Queue created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create Graphics Command Queue.\n");
			throw std::runtime_error("");
		}

		//Compute Queue
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_computeQueue));
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Compute Command Queue created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create Compute Command Queue.\n");
			throw std::runtime_error("");
		}

		//Transfer (Copy) Queue
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_transferQueue));
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Transfer Command Queue created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create Transfer Command Queue.\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}
	
	
	
	void NK::D3D12Device::CreateDescriptorHeaps()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating Descriptor Heaps\n");


		//----RESOURCE HEAP----//
		D3D12_DESCRIPTOR_HEAP_DESC resourceHeapDesc{};
		resourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		resourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		resourceHeapDesc.NumDescriptors = MAX_BINDLESS_RESOURCES;
		HRESULT hr{ m_device->CreateDescriptorHeap(&resourceHeapDesc, IID_PPV_ARGS(&m_resourceDescriptorHeap)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Resource Descriptor Heap created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create Resource Descriptor Heap.\n");
			throw std::runtime_error("");
		}
		m_resourceDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


		//----SAMPLER HEAP----//
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{};
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		samplerHeapDesc.NumDescriptors = MAX_BINDLESS_SAMPLERS;
		hr = m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap));
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Sampler Descriptor Heap created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create Sampler Descriptor Heap.\n");
			throw std::runtime_error("");
		}
		m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);



		m_logger.Unindent();
	}



	void NK::D3D12Device::CreateRootSignature()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating Root Signature\n");


		std::array<D3D12_ROOT_PARAMETER1, 2> rootParams;

		
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
		samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE; //samplers are usually a bit more static than resources
		samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &samplerRange;


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
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Root Signature serialisation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Root Signature serialisation unsuccessful - error blob: ");
			m_logger.IndentRawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, static_cast<const char*>(errorStr->GetBufferPointer()) + std::string("\n"));
			throw std::runtime_error("");
		}


		//Create root signature
		hr = m_device->CreateRootSignature(0, serialisedRootSig->GetBufferPointer(), serialisedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSig));
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Root Signature creation successful.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Root Signature creation unsuccessful\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}

}