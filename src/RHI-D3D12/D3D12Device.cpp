#include "D3D12Device.h"
#include "D3D12Device.h"
#include "D3D12Device.h"
#include "D3D12Device.h"
#include "D3D12Device.h"
#include <Core/Memory/Allocation.h>
#include <Core/Utils/FormatUtils.h>
#include <Core/Utils/EnumUtils.h>
#include "D3D12CommandPool.h"
#include "D3D12Buffer.h"
#include "D3D12BufferView.h"
#include "D3D12Texture.h"
#include "D3D12TextureView.h"
#include "D3D12Surface.h"
#include "D3D12Swapchain.h"
#include "D3D12Shader.h"
#include "D3D12Pipeline.h"
#include "D3D12Queue.h"
#include "D3D12Fence.h"
#include "D3D12Semaphore.h"
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
		RegisterCallback();
		CreateDescriptorHeaps();
		CreateRootSignature();
		CreateSyncLists();

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



	UniquePtr<IBufferView> D3D12Device::CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc)
	{
		return UniquePtr<IBufferView>(NK_NEW(D3D12BufferView, m_logger, *m_resourceIndexAllocator, *this, _buffer, _desc, m_resourceDescriptorHeap.Get(), m_resourceDescriptorSize));
	}



	UniquePtr<ITexture> D3D12Device::CreateTexture(const TextureDesc& _desc)
	{
		return UniquePtr<ITexture>(NK_NEW(D3D12Texture, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ITextureView> D3D12Device::CreateTextureView(ITexture* _texture, const TextureViewDesc& _desc)
	{
		return UniquePtr<ITextureView>(NK_NEW(D3D12TextureView, m_logger, m_allocator, *this, _texture, _desc, m_resourceDescriptorHeap.Get(), m_resourceDescriptorSize, m_resourceIndexAllocator.get()));
	}



	UniquePtr<ICommandPool> D3D12Device::CreateCommandPool(const CommandPoolDesc& _desc)
	{
		return UniquePtr<ICommandPool>(NK_NEW(D3D12CommandPool, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISurface> D3D12Device::CreateSurface(const SurfaceDesc& _desc)
	{
		return UniquePtr<ISurface>(NK_NEW(D3D12Surface, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISwapchain> D3D12Device::CreateSwapchain(const SwapchainDesc& _desc)
	{
		return UniquePtr<ISwapchain>(NK_NEW(D3D12Swapchain, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<IShader> D3D12Device::CreateShader(const ShaderDesc& _desc)
	{
		return UniquePtr<IShader>(NK_NEW(D3D12Shader, m_logger, _desc));
	}



	UniquePtr<IPipeline> D3D12Device::CreatePipeline(const PipelineDesc& _desc)
	{
		return UniquePtr<IPipeline>(NK_NEW(D3D12Pipeline, m_logger, m_allocator, *this, _desc));
	}

	
	
	UniquePtr<IQueue> D3D12Device::CreateQueue(const QueueDesc& _desc)
	{
		return UniquePtr<IQueue>(NK_NEW(D3D12Queue, m_logger, *this, _desc, m_syncLists[_desc.type].get()));
	}

	
	
	UniquePtr<IFence> D3D12Device::CreateFence(const FenceDesc& _desc)
	{
		return UniquePtr<IFence>(NK_NEW(D3D12Fence, m_logger, m_allocator, *this, _desc));
	}



	UniquePtr<ISemaphore> D3D12Device::CreateSemaphore()
	{
		return UniquePtr<ISemaphore>(NK_NEW(D3D12Semaphore, m_logger, m_allocator, *this));
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

		Microsoft::WRL::ComPtr<IDXGIFactory1> tempFactory;
		HRESULT result{ CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&tempFactory)) };
		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Factory successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create factory\n");
			throw std::runtime_error("");
		}

		result = tempFactory.As(&m_factory);
		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Factory successfully upgraded to DXGIFactory4\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to upgrade factory to DXGIFactory4\n");
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



	void D3D12Device::RegisterCallback()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Registering Debug Layer Message Callback\n");


		ID3D12InfoQueue1* infoQueue;
		const HRESULT hr{ m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)) };
		if (FAILED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to query for info queue interface - is the debug layer enabled?\n");
			throw std::runtime_error("");
		}

		DWORD callbackFuncUUID{ 0 };
		infoQueue->RegisterMessageCallback(D3D12Device::DebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &callbackFuncUUID);
		
		infoQueue->Release();


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



	void D3D12Device::CreateSyncLists()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::DEVICE, "Creating Sync Lists\n");


		CommandBufferDesc primaryLevelBufferDesc{};
		primaryLevelBufferDesc.level = COMMAND_BUFFER_LEVEL::PRIMARY;

		CommandPoolDesc poolDesc{};
		poolDesc.type = COMMAND_POOL_TYPE::GRAPHICS;
		m_graphicsSyncListAllocator = CreateCommandPool(poolDesc);
		poolDesc.type = COMMAND_POOL_TYPE::COMPUTE;
		m_computeSyncListAllocator = CreateCommandPool(poolDesc);
		poolDesc.type = COMMAND_POOL_TYPE::TRANSFER;
		m_transferSyncListAllocator = CreateCommandPool(poolDesc);

		m_syncLists[COMMAND_POOL_TYPE::GRAPHICS] = m_graphicsSyncListAllocator->AllocateCommandBuffer(primaryLevelBufferDesc);
		m_syncLists[COMMAND_POOL_TYPE::COMPUTE] = m_computeSyncListAllocator->AllocateCommandBuffer(primaryLevelBufferDesc);
		m_syncLists[COMMAND_POOL_TYPE::TRANSFER] = m_transferSyncListAllocator->AllocateCommandBuffer(primaryLevelBufferDesc);


		m_logger.Unindent();
	}



	void D3D12Device::DebugCallback(D3D12_MESSAGE_CATEGORY _category, D3D12_MESSAGE_SEVERITY _severity, D3D12_MESSAGE_ID _id, LPCSTR _pDescription, void* _pContext)
	{
		//pContext is the parent D3D12Device
		const D3D12Device* device{ reinterpret_cast<const D3D12Device*>(_pContext) };

		//Get channel
		LOGGER_CHANNEL channel;
		switch (_severity)
		{
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		case D3D12_MESSAGE_SEVERITY_ERROR:
			channel = LOGGER_CHANNEL::ERROR;
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			channel = LOGGER_CHANNEL::WARNING;
			break;
		case D3D12_MESSAGE_SEVERITY_INFO:
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
			channel = LOGGER_CHANNEL::INFO;
			break;
		default:
			channel = LOGGER_CHANNEL::NONE;
			break;
		}

		//Get layer
		LOGGER_LAYER layer;
		switch (_category)
		{
		case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:	layer = LOGGER_LAYER::D3D12_APP_DEFINED; break;
		case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:			layer = LOGGER_LAYER::D3D12_MISC; break;
		case D3D12_MESSAGE_CATEGORY_INITIALIZATION:			layer = LOGGER_LAYER::D3D12_INIT; break;
		case D3D12_MESSAGE_CATEGORY_CLEANUP:				layer = LOGGER_LAYER::D3D12_CLEANUP; break;
		case D3D12_MESSAGE_CATEGORY_COMPILATION:			layer = LOGGER_LAYER::D3D12_COMPILATION; break;
		case D3D12_MESSAGE_CATEGORY_STATE_CREATION:			layer = LOGGER_LAYER::D3D12_STATE_CREATE; break;
		case D3D12_MESSAGE_CATEGORY_STATE_SETTING:			layer = LOGGER_LAYER::D3D12_STATE_SET; break;
		case D3D12_MESSAGE_CATEGORY_STATE_GETTING:			layer = LOGGER_LAYER::D3D12_STATE_GET; break;
		case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:	layer = LOGGER_LAYER::D3D12_RES_MANIP; break;
		case D3D12_MESSAGE_CATEGORY_EXECUTION:				layer = LOGGER_LAYER::D3D12_EXECUTION; break;
		case D3D12_MESSAGE_CATEGORY_SHADER:					layer = LOGGER_LAYER::D3D12_SHADER; break;
		default:											layer = LOGGER_LAYER::UNKNOWN; break;
		}

		std::string msg{ std::to_string(std::to_underlying(_id)) + std::string(": ") + std::string(_pDescription)};
		if (msg.back() != '\n') { msg += '\n'; } //dx12 just like sometimes doesn't do this
		device->m_logger.IndentLog(channel, layer, msg);

		if (channel == LOGGER_CHANNEL::ERROR)
		{
			if (EnumUtils::Contains(device->m_logger.GetLoggerConfig().GetChannelBitfieldForLayer(layer), LOGGER_CHANNEL::ERROR))
			{
				throw std::runtime_error("");
			}
			else
			{
				throw std::runtime_error("Error thrown from " + device->m_logger.LayerToString(layer) + " but ERROR channel for this layer was disabled - enable it to view the error.\n");
			}
		}
	}

}