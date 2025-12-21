#include "ObjectComponent.h"
#include "GameEngine.h"
#include "GameObject.h"

float& ObjectComponent::fDt = GameEngine::fDt;

void AIComponent::SetOwner(Object* _owner)
{
    ObjectComponent::SetOwner(_owner);
    if (!_owner)
    {
        SYSTEM_LOG << "Error AIComponent::SetOwner called with null owner!\n";
        return;
    }
    if (owner->GetObjectType() != ObjectType::Entity)
    {
        SYSTEM_LOG << "Error AIComponent::SetOwner called with non-Entity owner!\n";
        return;
    }
}
//-------------------------------------------------------------
void ObjectComponent::OnEvent(const Message& msg)
{
    // Default ObjectComponent OnEvent - override in derived classes for specific handling
}
//-------------------------------------------------------------
void PhysicsComponent::OnEvent(const Message& msg)
{
    ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void AIComponent::OnEvent(const Message& msg)
{
    ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void VisualComponent::OnEvent(const Message& msg)
{
    ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void AudioComponent::OnEvent(const Message& msg)
{
	ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void SystemComponent::OnEvent(const Message& msg)
{
	ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void ProcessusComponent::OnEvent(const Message& msg)
{
	ObjectComponent::OnEvent(msg);
}
//-------------------------------------------------------------
void GameObjectComponent::OnEvent(const Message& msg)
{
	ObjectComponent::OnEvent(msg);
}
