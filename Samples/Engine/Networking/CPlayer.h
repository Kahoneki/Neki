#pragma once


enum class PLAYER_ACTIONS
{
	MOVE,
	JUMP,
};


struct CPlayer
{
	float movementSpeed;
};


struct PlayerJumpEvent
{
	NK::Entity entity;
};