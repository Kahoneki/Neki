#pragma once

#include <Core/Memory/Allocation.h>
#include <Graphics/Lights/Light.h>


namespace NK
{

	struct CLight final
	{
	public:
		LIGHT_TYPE lightType{ LIGHT_TYPE::UNDEFINED };
		UniquePtr<Light> light;
	};
	
}