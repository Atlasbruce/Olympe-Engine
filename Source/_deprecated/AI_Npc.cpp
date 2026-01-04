/*
	Purpose: Implementation of AI_Npc class for non-player character behavior in a game.
*/
#include "AI_Npc.h"
#include "GameObject.h"
#include "ObjectFactory.h"


bool AI_Npc::FactoryRegistered = ObjectFactory::Get().Register("AI_Npc", AI_Npc::Create);
ObjectComponent* AI_Npc::Create()
{
	return new AI_Npc();
}

AI_Npc::AI_Npc()
{
	// Constructor logic (if any) goes here

}

AI_Npc::~AI_Npc()
{
}

void AI_Npc::SetOwner(Object* _owner)
{
	if (!_owner)
	{
		SYSTEM_LOG << "Error AI_Npc::SetOwner called with null owner!\n";
		return;
	}
	owner = _owner;

}

void AI_Npc::OnEvent(const Message& msg)
{
}
