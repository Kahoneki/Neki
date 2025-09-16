#include "D3D12Semaphore.h"
#include <stdexcept>
#include <Core/Utils/FormatUtils.h>
#ifdef ERROR
	#undef ERROR
#endif

namespace NK
{

	D3D12Semaphore::D3D12Semaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device)
	: ISemaphore(_logger, _allocator, _device)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SEMAPHORE, "Initialising D3D12Semaphore\n");


		//Create the fence - a "semaphore" in D3D12 is just a fence that's used exclusively for GPU-GPU syncing.
		//It always starts at 0
		const HRESULT hr{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SEMAPHORE, "ID3D12Fence initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SEMAPHORE, "ID3D12Fence initialisation unsuccessful. result = " + std::to_string(hr) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	D3D12Semaphore::~D3D12Semaphore()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SEMAPHORE, "Shutting Down D3D12Semaphore\n");

		
		//ComPtrs are released automatically

		
		m_logger.Unindent();
	}
	
}