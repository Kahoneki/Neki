#pragma once

#include "CPlayer.h"

#include <Core/Layers/ILayer.h>
#include <Core-ECS/Registry.h>
#include <Graphics/Window.h>
#include <Managers/EventManager.h>
#include <Types/NekiTypes.h>



class PlayerLayer final : public NK::ILayer
{
public:
	explicit PlayerLayer(NK::Registry& _reg);
	virtual ~PlayerLayer() override;

	virtual void Update() override;


private:
	NK::EventSubscriptionID m_playerJumpEventSubscriptionID;
	void HandleJump(const PlayerJumpEvent& _event);
};