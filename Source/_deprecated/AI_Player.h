#pragma once

#include "ObjectComponent.h"
#include "system/EventManager.h"
#include <SDL3/SDL.h>
#include <mutex>

// AI_Player: component that listens to input events (keyboard / joystick)
// and moves its owner GameObject in the four cardinal directions.
class AI_Player : public AIComponent
{
public:
    static bool FactoryRegistered;
    static ObjectComponent* Create(void);

    explicit AI_Player();
    virtual void Initialize() override;
    virtual ~AI_Player() override;

	virtual void SetOwner(Object* _owner) override;

    virtual void RenderDebug();

    // AI properties participate in the AI stage (AIProperty already does this)
    virtual void Process() override;
    virtual void OnEvent(const Message& msg) override;

private:
	SDL_Color m_debugcolor = { 0, 255, 0, 255 };

    float m_speed = 250.0f; // for tests to be removed

    // input state
    float m_axisX = 0.0f;
    float m_axisY = 0.0f;
    bool m_keyUp = false;
    bool m_keyDown = false;
    bool m_keyLeft = false;
    bool m_keyRight = false;

    float m_deadzone = 0.2f;
    std::mutex m_mutex;
};
