#include "RenderSystem.h"

#ifdef NEKI_VULKAN_SUPPORTED
	#include "RHI-Vulkan/VulkanDevice.h"
#endif
#ifdef NEKI_D3D12_SUPPORTED
	#include "RHI-D3D12/D3D12Device.h"
#endif


namespace NK
{

	RenderSystem::RenderSystem(ILogger& _logger, IAllocator& _allocator, const RenderSystemDesc& _desc)
	: m_logger(_logger), m_allocator(_allocator), m_backend(_desc.backend), m_msaaEnabled(_desc.enableMSAA), m_msaaSampleCount(_desc.msaaSampleCount), m_ssaaEnabled(_desc.enableSSAA), m_ssaaMultiplier(_desc.ssaaMultiplier), m_framesInFlight(_desc.framesInFlight)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::RENDER_SYSTEM, "Initialising Render System\n");


		//Initialise device
		switch (m_backend)
		{
		case GRAPHICS_BACKEND::VULKAN:
		{
			#ifdef NEKI_VULKAN_SUPPORTED
				m_device = UniquePtr<IDevice>(NK_NEW(VulkanDevice, m_logger, m_allocator));
			#else
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend = GRAPHICS_BACKEND::VULKAN but compiler definition NEKI_VULKAN_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
				throw std::invalid_argument("");
			#endif
			break;
		}
		case GRAPHICS_BACKEND::D3D12:
		{
			#ifdef NEKI_D3D12_SUPPORTED
				m_device = UniquePtr<IDevice>(NK_NEW(D3D12Device, m_logger, m_allocator));
			#else
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend = GRAPHICS_BACKEND::D3D12 but compiler definition NEKI_D3D12_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
				throw std::invalid_argument("");
			#endif
			break;
		}
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend (" + std::to_string(std::to_underlying(_desc.backend)) + ") not recognised.\n");
			throw std::invalid_argument("");
		}
		}


		m_logger.Unindent();
	}



	RenderSystem::~RenderSystem()
	{
		
	}

}