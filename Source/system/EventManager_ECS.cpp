#include "EventManager.h"
#include "../../source/ECS_Components.h"
#include "../World.h"
#include "../InputsManager.h"

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
            // Use optimized input entity cache instead of iterating all entities
            const auto& inputEntities = InputsManager::Get().GetInputEntities();
            for (EntityID entity : inputEntities)
            {
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
                        if (button >=0 && button < Controller_data::MAX_BUTTONS) ctrl.buttons[button] = (msg.state != 0);
                    }

                    // Axis motion: update Controller_data axes
                    if (msg.msg_type == EventType::Olympe_EventType_Joystick_AxisMotion)
                    {
                        int axis = msg.controlId;
                        float value = msg.param1; // normalized [-1,1]
                        // Map axes to Controller_data structure
                        // axis 0 -> leftStick.x, axis 1 -> leftStick.y
                        // axis 2 -> rightStick.x, axis 3 -> rightStick.y
                        // axis 4 -> leftTrigger, axis 5 -> rightTrigger
                        if (axis == 0) ctrl.leftStick.x = value;
                        else if (axis == 1) ctrl.leftStick.y = value;
                        else if (axis == 2) ctrl.rightStick.x = value;
                        else if (axis == 3) ctrl.rightStick.y = value;
                        else if (axis == 4) ctrl.leftTrigger = (value + 1.0f) * 0.5f; // normalize -1..1 to 0..1
                        else if (axis == 5) ctrl.rightTrigger = (value + 1.0f) * 0.5f;
                    }

                    // Keyboard events are now handled by InputMappingSystem via Pull API
                    // No longer need to process keyboard events here

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
