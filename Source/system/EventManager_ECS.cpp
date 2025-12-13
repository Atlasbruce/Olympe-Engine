#include "EventManager.h"
#include "../../source/ECS_Components.h"
#include "../World.h"

// NOTE: This translation unit implements the helper that updates ECS components
// based on input messages. It is separated to avoid circular includes.

void UpdateECSInputFromMessage(const Message& msg)
{
    using namespace std;

    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Joystick_AxisMotion:
        case EventType::Olympe_EventType_Joystick_ButtonDown:
        case EventType::Olympe_EventType_Joystick_ButtonUp:
        case EventType::Olympe_EventType_Joystick_Connected:
        case EventType::Olympe_EventType_Joystick_Disconnected:
        case EventType::Olympe_EventType_Keyboard_KeyDown:
        case EventType::Olympe_EventType_Keyboard_KeyUp:
        case EventType::Olympe_EventType_Keyboard_Connected:
        case EventType::Olympe_EventType_Keyboard_Disconnected:
        case EventType::Olympe_EventType_Mouse_ButtonDown:
        case EventType::Olympe_EventType_Mouse_ButtonUp:
        case EventType::Olympe_EventType_Mouse_Motion:
        case EventType::Olympe_EventType_Mouse_Wheel:
        {
            // Iterate over all entities that have PlayerBinding_data
            auto &signatures = World::Get().m_entitySignatures;
            for (const auto &kv : signatures)
            {
                EntityID entity = kv.first;
                try
                {
                    if (!World::Get().HasComponent<PlayerBinding_data>(entity)) continue;
                    PlayerBinding_data &binding = World::Get().GetComponent<PlayerBinding_data>(entity);

                    // Match device id: for keyboard, joystickID may be -1
                    if (binding.controllerID != msg.deviceId) continue;

                    // Ensure Controller_data exists
                    if (!World::Get().HasComponent<Controller_data>(entity))
                    {
                        continue;
                        //World::Get().AddComponent<Controller_data>(entity);
                    }
                    Controller_data &ctrl = World::Get().GetComponent<Controller_data>(entity);
                    ctrl.controllerID = static_cast<short>(msg.deviceId);

                    // Update connection state
                    if (msg.msg_type == EventType::Olympe_EventType_Joystick_Connected || msg.msg_type == EventType::Olympe_EventType_Keyboard_Connected)
                        ctrl.isConnected = true;
                    if (msg.msg_type == EventType::Olympe_EventType_Joystick_Disconnected || msg.msg_type == EventType::Olympe_EventType_Keyboard_Disconnected)
                        ctrl.isConnected = false;

                    // Button events
                    if (msg.msg_type == EventType::Olympe_EventType_Joystick_ButtonDown || msg.msg_type == EventType::Olympe_EventType_Joystick_ButtonUp)
                    {
                        int button = msg.controlId;
                        if (button >=0 && button < 30) ctrl.buttonStates[button] = (msg.state != 0);
                    }

                    // Axis motion: update PlayerController_data Joydirection
                    if (msg.msg_type == EventType::Olympe_EventType_Joystick_AxisMotion)
                    {
                        if (!World::Get().HasComponent<PlayerController_data>(entity))
                        {
                            continue;
                            //World::Get().AddComponent<PlayerController_data>(entity);
                        }
                        PlayerController_data &pctrl = World::Get().GetComponent<PlayerController_data>(entity);
                        int axis = msg.controlId;
                        float value = msg.param1; // normalized [-1,1]
                        // Simple mapping: axis 0 -> x, axis 1 -> y
                        if (axis == 0) pctrl.Joydirection.x = value;
                        else if (axis == 1) pctrl.Joydirection.y = value;
                    }

                    // Keyboard example mapping
                    if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyDown || msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyUp)
                    {
                        if (!World::Get().HasComponent<PlayerController_data>(entity))
                        {
                            continue;
                            //World::Get().AddComponent<PlayerController_data>(entity);
                        }
                        PlayerController_data &pctrl = World::Get().GetComponent<PlayerController_data>(entity);

                        switch(msg.msg_type)
						{
                            case EventType::Olympe_EventType_Keyboard_KeyDown:
                            {
                                // msg.controlId contains SDL_Scancode
                                switch (static_cast<SDL_Scancode>(msg.controlId))
                                {
                                case SDL_SCANCODE_Z:
                                case SDL_SCANCODE_UP:
                                    pctrl.Joydirection.y = -1.f; break;
                                case SDL_SCANCODE_S:
                                case SDL_SCANCODE_DOWN:
                                    pctrl.Joydirection.y = 1.f; break;
                                case SDL_SCANCODE_Q:
                                case SDL_SCANCODE_LEFT:
                                    pctrl.Joydirection.x = -1.f; break;
                                case SDL_SCANCODE_D:
                                case SDL_SCANCODE_RIGHT:
                                    pctrl.Joydirection.x = 1.f; break;
                                default:
                                    break;
                                }
                                break;
                            }
                            case EventType::Olympe_EventType_Keyboard_KeyUp:
                            {
                                switch (static_cast<SDL_Scancode>(msg.controlId))
                                {
                                case SDL_SCANCODE_W:
                                case SDL_SCANCODE_UP:
                                case SDL_SCANCODE_S:
                                case SDL_SCANCODE_DOWN:
                                    pctrl.Joydirection.y = 0.f; break;
                                case SDL_SCANCODE_A:
                                case SDL_SCANCODE_LEFT:
                                case SDL_SCANCODE_D:
                                case SDL_SCANCODE_RIGHT:
                                    pctrl.Joydirection.x = 0.f; break;
                                default:
                                    break;
                                }
                                break;
                            }
                        }

                        //if (msg.controlId == 32) // space -> jump
                        //    pctrl.isJumping = (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyDown);
                    }

					// DEPRECATED MESSAGE HERE THE COMPONENTS WILL PRCOESS THEMSELVES AND RETRIEVE DATA AS NEEDED
                    // 
                    //// After updating components, notify the entity/player by dispatching a targeted message
                    //Message targeted = msg;
                    //targeted.targetUid = entity;
                    //// Immediate dispatch so players can read up-to-date component data
                    //EventManager::Get().DispatchImmediate(targeted);
                }
                catch (const std::exception&)
                {
                    // ignore per-entity errors
                }
            }
            break;
        }
        default:
            break;
    }
}
