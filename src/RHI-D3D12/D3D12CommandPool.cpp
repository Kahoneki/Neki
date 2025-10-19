#include "D3D12CommandPool.h"

#include "D3D12CommandBuffer.h"

#include <Core/Memory/Allocation.h>

#include <stdexcept>
#include <utility>


namespace NK
{

	D3D12CommandPool::D3D12CommandPool(ILogger& _logger, IAllocator& _allocator, D3D12Device& _device, const CommandPoolDesc& _desc)
		: ICommandPool(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Initialising D3D12CommandPool\n");

		//Create the pool
		D3D12_COMMAND_LIST_TYPE type;
		switch (m_type)
		{
		case COMMAND_TYPE::GRAPHICS:	type = D3D12_COMMAND_LIST_TYPE_DIRECT;	break;
		case COMMAND_TYPE::COMPUTE:		type = D3D12_COMMAND_LIST_TYPE_COMPUTE;	break;
		case COMMAND_TYPE::TRANSFER:	type = D3D12_COMMAND_LIST_TYPE_COPY;	break;
		}
		const HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_pool)) };

		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_POOL, "Initialisation successful - type = " + GetPoolTypeString() + '\n');
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	D3D12CommandPool::~D3D12CommandPool()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Shutting Down D3D12CommandPool\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	UniquePtr<ICommandBuffer> D3D12CommandPool::AllocateCommandBuffer(const CommandBufferDesc& _desc)
	{
		if (_desc.level == COMMAND_BUFFER_LEVEL::PRIMARY)
		{
			return UniquePtr<ICommandBuffer>(NK_NEW(D3D12CommandBuffer, m_logger, m_device, *this, _desc));
		}
		else
		{
			//todo: look into ID3D12Bundle and implement
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "SECONDARY COMMAND BUFFERS FOR D3D12 (BUNDLES) NOT YET IMPLEMENTED\n");
			throw std::runtime_error("");
		}
	}



	void D3D12CommandPool::Reset(COMMAND_POOL_RESET_FLAGS _type)
	{
		//todo: implement
	}



	std::string D3D12CommandPool::GetPoolTypeString() const
	{
		switch (m_type)
		{
		case COMMAND_TYPE::GRAPHICS:	return "GRAPHICS";
		case COMMAND_TYPE::COMPUTE:		return "COMPUTE";
		case COMMAND_TYPE::TRANSFER:	return "TRANSFER";
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "GetPoolTypeString() - switch case returned default. type = " + std::to_string(std::to_underlying(m_type)));
			throw std::runtime_error("");
		}
		}
	}

}