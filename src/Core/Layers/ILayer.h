#pragma once

#include <Core/Debug/ILogger.h>
#include <Core-ECS/Registry.h>
#include <Types/NekiTypes.h>


namespace NK
{
	
	class ILayer
	{
	public:
		explicit ILayer(ILogger& _logger) : m_logger(_logger) {}
		virtual ~ILayer() = default;

		virtual void Update(Registry& _reg) = 0;


	protected:
		//Dependency injections
		ILogger& m_logger;
	};

}