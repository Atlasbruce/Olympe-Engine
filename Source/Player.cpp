/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing a player in the game, derived from GameObject.
- Handles Joypads and Input Mapping.
- Manages Events for collisions, interactions, and state changes.

*/

#include "Player.h"
#include "ObjectFactory.h"
#include "Sprite.h"
bool Player::FactoryRegistered = ObjectFactory::Get().Register("Player", Player::Create);
Object* Player::Create()
{
	// Create a new Player instance each time (not static)
	Player *player = new Player();
	player->name = "Player";
	ObjectFactory::Get().AddComponent("AI_Player", player);
	_Sprite* sprite = (_Sprite*)ObjectFactory::Get().AddComponent("_Sprite", player);
	if (SDL_rand(2) == 0)
		sprite->SetSprite("player_entity_male", "Resources/entity_male.png");
	else
		sprite->SetSprite("player_entity_female", "Resources/entity_female.png");

	return player;
}