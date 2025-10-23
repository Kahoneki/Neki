#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct InputLayerDesc
	{
		const Window* window;
	};
	
	
	class InputLayer final : public ILayer
	{
	public:
		explicit InputLayer(const InputLayerDesc& _desc);
		virtual ~InputLayer() override;

		virtual void Update(Registry& _reg) override;
		
		
	private:
		InputLayerDesc m_desc;
	};

}