#include "D3D12Device.h"
#include <stdexcept>
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

		m_logger.Unindent();
	}



	D3D12Device::~D3D12Device()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::DEVICE, "Shutting Down D3D12Device\n");
		
		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	UniquePtr<ICommandPool> D3D12Device::CreateCommandPool(const CommandPoolDesc& _desc)
	{
		//todo: implement
		return UniquePtr<ICommandPool>();
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
}