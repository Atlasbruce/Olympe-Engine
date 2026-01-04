/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing a Non-Player Character (NPC) in the game, derived from GameObject.


*/
#pragma once
#include "GameObject.h"
class Npc :
    public GameObject
{
public:

    Npc() = default;
	virtual ~Npc() override = default;
	virtual EntityType GetEntityType() const override { return EntityType::NPC; }

	static bool FactoryRegistered;
	static Object* Create();
};

