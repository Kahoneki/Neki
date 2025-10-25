#pragma once

#include "ILayer.h"

#include <Components/CCamera.h>


namespace NK
{
	
	class DiskModelLoaderLayer final : public ILayer
	{
	public:
		explicit DiskModelLoaderLayer();
		virtual ~DiskModelLoaderLayer() override = default;

		virtual void Update(Registry& _reg) override;
		
		
	private:
		ViewFrustum m_frustum;
	};

}