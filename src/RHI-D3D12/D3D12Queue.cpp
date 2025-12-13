#include "D3D12Queue.h"

#include "D3D12CommandBuffer.h"
#include "D3D12Fence.h"
#include "D3D12Semaphore.h"

#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


namespace NK
{

	D3D12Queue::D3D12Queue(ILogger& _logger, IDevice& _device, const QueueDesc& _desc, ICommandBuffer* _syncList)
	: IQueue(_logger, _device, _desc), m_syncList(_syncList)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::QUEUE, "Initialising D3D12Queue\n");


		//Create the queue
		D3D12_COMMAND_QUEUE_DESC desc{};
		switch (m_type)
		{
		case COMMAND_TYPE::GRAPHICS:	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
		case COMMAND_TYPE::COMPUTE:		desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
		case COMMAND_TYPE::TRANSFER:	desc.Type = D3D12_COMMAND_LIST_TYPE_COPY; break;
		}
		const HRESULT hr{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::DEVICE, "Command queue created.\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::DEVICE, "Failed to create command queue.\n");
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	D3D12Queue::~D3D12Queue()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down D3D12Queue\n");

		
		//ComPtrs are released automatically
		

		m_logger.Unindent();
	}



	void D3D12Queue::Submit(ICommandBuffer* _cmdBuffer, ISemaphore* _waitSemaphore, ISemaphore* _signalSemaphore, IFence* _signalFence)
	{
		//Handle wait semaphore
		if (_waitSemaphore)
		{
			D3D12Semaphore* d3d12WaitSemaphore{ dynamic_cast<D3D12Semaphore*>(_waitSemaphore) };
			if (d3d12WaitSemaphore->GetFenceValue() > 0)
			{
				m_queue->Wait(d3d12WaitSemaphore->GetFence(), d3d12WaitSemaphore->GetFenceValue());
			}
		}

		//Execute command list
		D3D12CommandBuffer* d3d12CommandBuffer{ dynamic_cast<D3D12CommandBuffer*>(_cmdBuffer) };
		ID3D12CommandList* commandList[]{ d3d12CommandBuffer->GetCommandList() };
		m_queue->ExecuteCommandLists(1, commandList);

		//Handle signal semaphore
		if (_signalSemaphore)
		{
			D3D12Semaphore* d3d12SignalSemaphore{ dynamic_cast<D3D12Semaphore*>(_signalSemaphore) };
			//The next submit will wait for the next fence value
			d3d12SignalSemaphore->IncrementFenceValue();
			m_queue->Signal(d3d12SignalSemaphore->GetFence(), d3d12SignalSemaphore->GetFenceValue());
		}

		//Handle signal fence
		if (_signalFence)
		{
			D3D12Fence* d3d12SignalFence{ dynamic_cast<D3D12Fence*>(_signalFence) };
			
			//Ensure fence has been reset
			if (d3d12SignalFence->GetState() != FENCE_STATE::UNSIGNALLED)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::QUEUE, "Attempted to pass a fence to IQueue::Submit() that hasn't been Reset()\n");
				throw std::runtime_error("");
			}

			//The next IFence::Wait() call will wait for the next fence value
			d3d12SignalFence->IncrementFenceValue();
			m_queue->Signal(d3d12SignalFence->GetFence(), d3d12SignalFence->GetFenceValue());

			//Transition the state so it can have Wait() called on it
			d3d12SignalFence->SetState(FENCE_STATE::IN_FLIGHT);
		}
	}



	void D3D12Queue::WaitIdle()
	{
		//Submit an empty submit on m_syncList that signals a fence on its completion, then wait for that fence to complete
		NK::FenceDesc syncFenceDesc{};
		syncFenceDesc.initiallySignaled = false;
		NK::UniquePtr<NK::IFence> syncFence{ m_device.CreateFence(syncFenceDesc) };
		Submit(m_syncList, nullptr, nullptr, syncFence.get());
		syncFence->Wait();
		syncFence->Reset();
	}

}