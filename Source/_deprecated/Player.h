/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing a player in the game, derived from GameObject.
- Handles Joypads and Input Mapping.
- Manages Events for collisions, interactions, and state changes.

*/
#pragma once
#include "GameObject.h"
class Player : public GameObject
{
    public:
    Player() = default;
    virtual ~Player() override = default;
	virtual EntityType GetEntityType() const override { return EntityType::Player; }

	static bool FactoryRegistered;
	static Object* Create();

	int m_PlayerID = -1; // unique player ID assigned to this Player instance
	int m_ControllerID = -1; // controller instance ID assigned to this Player instance
	string m_PlayerName = "Unnamed_Player";
};

