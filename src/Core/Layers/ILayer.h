#pragma once

#include <Core/Debug/ILogger.h>
#include <Core-ECS/Registry.h>
#include <Types/NekiTypes.h>


namespace NK
{
	
	class ILayer
	{
	public:
		explicit ILayer() : m_logger(*Context::GetLogger()) {}
		virtual ~ILayer() = default;

		virtual void Update(Registry& _reg) = 0;


	protected:
		//Dependency injections
		ILogger& m_logger;
	};

}