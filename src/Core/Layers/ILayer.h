#pragma once

#include <Core/Debug/ILogger.h>
#include <Core-ECS/Registry.h>
#include <Types/NekiTypes.h>


namespace NK
{
	
	class ILayer
	{
	public:
		explicit ILayer(Registry& _reg) : m_logger(*Context::GetLogger()), m_reg(_reg) {}
		virtual ~ILayer() = default;

		inline void SetRegistry(Registry& _reg) { m_reg = _reg; }
		
		virtual void Update() {}
		inline virtual void FixedUpdate() {}


	protected:
		//Dependency injections
		ILogger& m_logger;

		std::reference_wrapper<Registry> m_reg;
	};

}