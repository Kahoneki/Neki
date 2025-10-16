#include "D3D12Fence.h"

#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


namespace NK
{

	D3D12Fence::D3D12Fence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc)
	: IFence(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::FENCE, "Initialising D3D12Fence\n");


		m_state = _desc.initiallySignaled ? FENCE_STATE::SIGNALLED : FENCE_STATE::UNSIGNALLED;
		m_fenceValue = _desc.initiallySignaled ? 1 : 0;

		const HRESULT hr{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)) };
		if (SUCCEEDED(hr))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::FENCE, "ID3D12Fence initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "ID3D12Fence initialisation unsuccessful. result = " + std::to_string(hr) + '\n');
			throw std::runtime_error("");
		}

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "Failed to create fence event handle\n");
		}

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::FENCE, "D3D12Fence initialisation successful\n");


		m_logger.Unindent();
	}



	D3D12Fence::~D3D12Fence()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::FENCE, "Shutting Down D3D12Fence\n");


		if (m_fenceEvent)
		{
			CloseHandle(m_fenceEvent);
			m_fenceEvent = nullptr;
		}

		//ComPtrs are released automatically


		m_logger.Unindent();
	}



	void D3D12Fence::Wait()
	{
		//If already signalled, don't need to do anything
		if (m_state == FENCE_STATE::SIGNALLED)
		{
			return;
		}

		//As soon as the queue is submitted by the cpu, the provided fence is moved to the in-flight state
		//This means that if the fence is in the unsignalled state, there is no active queue which would lead to an infinite wait
		//Throw an error instead of letting that happen^
		if (m_state == FENCE_STATE::UNSIGNALLED)
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "Wait() called on a fence that was not in-flight - this would have resulted in an infinite wait\n");
			throw std::runtime_error("");
		}

		//Check that the GPU hasn't already passed the point we're waiting for
		if (m_fence->GetCompletedValue() < m_fenceValue)
		{
			//Block cpu execution until m_fence reaches m_fenceValue
			HRESULT hr = m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
			if (FAILED(hr))
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "SetEventOnCompletion failed.\n");
				throw std::runtime_error("");
			}
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		//Wait is complete, execution has finished - update state
		m_state = FENCE_STATE::SIGNALLED;
	}



	void D3D12Fence::Reset()
	{
		//Enforce strict state checks for parity with vulkan - a fence can only be reset in the signalled state
		if (m_state != FENCE_STATE::SIGNALLED)
		{
			//Give a different error message based on what the likely scenario is
			if (m_state == FENCE_STATE::IN_FLIGHT)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "Attempted to call Reset() on a fence that was not in the signalled state (state = IN_FLIGHT). Did you forget to Wait()?\n");
				throw std::runtime_error("");
			}
			else
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "Attempted to call Reset() on a fence that was not in the signalled state (state = UNSIGNALLED). Did you try to Reset() it twice?\n");
				throw std::runtime_error("");
			}
		}

		//Reset state
		m_state = FENCE_STATE::UNSIGNALLED;
	}

}