#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>


namespace NK
{
	
	class PlayerCameraLayer final : public ILayer
	{
	public:
		explicit PlayerCameraLayer(Registry& _reg);
		virtual ~PlayerCameraLayer() override;

		virtual void Update() override;
	};

}