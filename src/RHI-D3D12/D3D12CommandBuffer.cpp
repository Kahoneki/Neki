#include "D3D12CommandBuffer.h"
#include "D3D12CommandPool.h"
#include "D3D12Device.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR //conflicts with LOGGER_CHANNEL::ERROR
#endif

namespace NK
{

	D3D12CommandBuffer::D3D12CommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
		: ICommandBuffer(_logger, _device, _pool, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Initialising D3D12CommandBuffer\n");

		//Create the command buffer
		D3D12_COMMAND_LIST_TYPE type;
		switch (m_pool.GetPoolType())
		{
		case COMMAND_POOL_TYPE::GRAPHICS:	type = D3D12_COMMAND_LIST_TYPE_DIRECT;	break;
		case COMMAND_POOL_TYPE::COMPUTE:	type = D3D12_COMMAND_LIST_TYPE_COMPUTE;	break;
		case COMMAND_POOL_TYPE::TRANSFER:	type = D3D12_COMMAND_LIST_TYPE_COPY;	break;
		}
		const HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommandList(0, type, dynamic_cast<D3D12CommandPool&>(m_pool).GetCommandPool(), nullptr, IID_PPV_ARGS(&m_buffer)) };

		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_BUFFER, "Initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_BUFFER, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	D3D12CommandBuffer::~D3D12CommandBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Shutting Down D3D12CommandBuffer\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	void D3D12CommandBuffer::Reset()
	{
		//todo: implement
	}
}