#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct InputLayerDesc
	{
		explicit InputLayerDesc(const Window* _window) : window(_window) {}
		const Window* window;
	};
	
	
	class InputLayer final : public ILayer
	{
	public:
		explicit InputLayer(Registry& _reg, const InputLayerDesc& _desc);
		virtual ~InputLayer() override;

		virtual void Update() override;
		
		
	private:
		InputLayerDesc m_desc;
	};

}