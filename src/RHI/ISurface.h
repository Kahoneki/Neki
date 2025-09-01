#pragma once
#include "IDevice.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

namespace NK
{
	struct SurfaceDesc
	{
		std::string name; //name of the window
		glm::ivec2 size; //size of the surface in pixels
	};
	
	
	class ISurface
	{
	public:
		virtual ~ISurface() = default;


	protected:
		explicit ISurface(ILogger& _logger,IAllocator& _allocator, IDevice& _device, const SurfaceDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_name(_desc.name), m_size(_desc.size), m_window(nullptr) {}
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		std::string m_name;
		glm::ivec2 m_size;

		GLFWwindow* m_window;
	};
	
}