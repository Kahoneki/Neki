#pragma once

#include "ILayer.h"

#include <Components/CCamera.h>


namespace NK
{
	
	class ModelVisibilityLayer final : public ILayer
	{
	public:
		explicit ModelVisibilityLayer(Registry& _reg);
		virtual ~ModelVisibilityLayer() override = default;

		virtual void Update() override;
		
		
	private:
		ViewFrustum m_frustum;
	};

}