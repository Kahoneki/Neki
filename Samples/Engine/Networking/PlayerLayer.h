#pragma once

#include <Core/Layers/ILayer.h>
#include <Core-ECS/Registry.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>



class PlayerLayer final : public NK::ILayer
{
public:
	explicit PlayerLayer(NK::Registry& _reg);
	virtual ~PlayerLayer() override;

	virtual void Update() override;
};