#pragma once

#include <Core/Memory/Allocation.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>

#include <RHI/ICommandBuffer.h>
#include <RHI/IDevice.h>


namespace NK
{

	struct RenderSystemDesc
	{
		GRAPHICS_BACKEND backend;
		
		bool enableMSAA{ false };
		SAMPLE_COUNT msaaSampleCount{ SAMPLE_COUNT::BIT_1 };
		
		bool enableSSAA{ false };
		std::uint32_t ssaaMultiplier{ 1 };

		WindowDesc windowDesc;

		std::uint32_t framesInFlight;
	};

	
	class RenderSystem final
	{
	public:
		RenderSystem(ILogger& _logger, IAllocator& _allocator, const RenderSystemDesc& _desc);
		~RenderSystem();
		
		
	private:
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		
		GRAPHICS_BACKEND m_backend;
		
		bool m_msaaEnabled;
		SAMPLE_COUNT m_msaaSampleCount;

		bool m_ssaaEnabled;
		std::uint32_t m_ssaaMultiplier;

		std::uint32_t m_framesInFlight;

		UniquePtr<IDevice> m_device;
	};

}