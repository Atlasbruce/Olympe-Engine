#pragma once
#include "ObjectComponent.h"
#include "system/EventManager.h"
#include <SDL3/SDL.h>
#include <mutex>
// AI_Npc: component that implements basic NPC behavior (e.g., patrolling)
class AI_Npc : public AIComponent
{
	public:
	static bool FactoryRegistered;
	static ObjectComponent* Create(void);
	explicit AI_Npc();
	virtual ~AI_Npc() override;
	virtual void SetOwner(Object* _owner) override;
	// AI properties participate in the AI stage (AIComponent already does this)
	virtual void Process() override {}
	virtual void OnEvent(const Message& msg) override;
private:
	// NPC behavior state
	enum class State
	{
		Idle,
		Patrolling,
		Chasing,
		Fleeing
	};
	State m_currentState = State::Idle;
	std::mutex m_mutex;
};
