#pragma once

#include "ILayer.h"


namespace NK
{
	
	class WindowLayer final : public ILayer
	{
	public:
		explicit WindowLayer(Registry& _reg);
		virtual ~WindowLayer() override = default;

		virtual void Update() override;
	};

}