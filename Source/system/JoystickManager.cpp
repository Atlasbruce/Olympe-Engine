#include "JoystickManager.h"
#include "EventManager.h"
#include "system_utils.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <cstring>
#include "../InputsManager.h"
#include "message.h"
using IM = ::InputsManager;
using EM = ::EventManager;

//---------------------------------------------------------------------------------------------
JoystickManager& JoystickManager::GetInstance()
{
	static JoystickManager* instance = new JoystickManager();
    return *instance;
}
//---------------------------------------------------------------------------------------------
void JoystickManager::BeginFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Reset per-frame button edge detection for all joysticks
    for (auto& kv : m_joyStates)
    {
        std::memset(kv.second.buttonsPressed, 0, sizeof(kv.second.buttonsPressed));
        std::memset(kv.second.buttonsReleased, 0, sizeof(kv.second.buttonsReleased));
    }
}
//---------------------------------------------------------------------------------------------
bool JoystickManager::GetButton(SDL_JoystickID id, int button) 
{
    if (button < 0 || button >= MAX_BUTTONS) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_joyStates.find(id);
    if (it == m_joyStates.end()) return false;
    return it->second.buttons[button];
}
//---------------------------------------------------------------------------------------------
bool JoystickManager::IsButtonPressed(SDL_JoystickID id, int button) 
{
    if (button < 0 || button >= MAX_BUTTONS) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_joyStates.find(id);
    if (it == m_joyStates.end()) return false;
    return it->second.buttonsPressed[button];
}
//---------------------------------------------------------------------------------------------
bool JoystickManager::IsButtonReleased(SDL_JoystickID id, int button) 
{
    if (button < 0 || button >= MAX_BUTTONS) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_joyStates.find(id);
    if (it == m_joyStates.end()) return false;
    return it->second.buttonsReleased[button];
}
//---------------------------------------------------------------------------------------------
float JoystickManager::GetAxis(SDL_JoystickID id, int axis) 
{
    if (axis < 0 || axis >= MAX_AXES) return 0.0f;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_joyStates.find(id);
    if (it == m_joyStates.end()) return 0.0f;
    return it->second.axes[axis];
}
//---------------------------------------------------------------------------------------------
void JoystickManager::Initialize()
{
  //  std::lock_guard<std::mutex> lock(m_mutex);

    // Enable joystick events so SDL will post them to the event queue
    SDL_SetJoystickEventsEnabled(true);

    Scan_Joysticks();
	SYSTEM_LOG << "JoystickManager created and Initialized with " << m_joysticks.size() << " joysticks connected\n";
}
//---------------------------------------------------------------------------------------------
// Scan currently connected joysticks and open them
void JoystickManager::Scan_Joysticks()
{
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    if (ids)
    {
        for (int i = 0; i < count; ++i)
        {
            OpenJoystick(ids[i]);
        }
        SDL_free(ids);
    }
}
//---------------------------------------------------------------------------------------------
void JoystickManager::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &kv : m_joysticks)
    {
        if (kv.second.joystick)
        {
            SDL_CloseJoystick(kv.second.joystick);
        }
    }
    m_joysticks.clear();
	SYSTEM_LOG << "JoystickManager Shutdown\n";
}
//---------------------------------------------------------------------------------------------
void JoystickManager::Process(float /*dt*/)
{
    // optional polling-based updates could be implemented here
    // For now, joystick activity is driven by SDL events handled in HandleEvent().
}
//---------------------------------------------------------------------------------------------
void JoystickManager::HandleEvent(const SDL_Event* ev)
{
    //if (!ev) return; FBind// useless because it is tested before function call

    switch (ev->type)
    {
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            // Gamepad added - we can open the underlying joystick if needed
            SDL_JoystickID id = ev->gdevice.which;
            OpenJoystick(id);
            SYSTEM_LOG << "Gamepad added id=" << id << "\n";
            break;
		}
        case SDL_EVENT_JOYSTICK_ADDED:
        {
            SDL_JoystickID id = ev->jdevice.which;
            OpenJoystick(id);
            SYSTEM_LOG << "Joystick added id=" << id << "\n";

            //Notify players that their controller has been disconnected
            EM::Get().DispatchImmediate
            (Message::Create
            (
                EventStructType::EventStructType_Olympe,
                EventType::Olympe_EventType_Joystick_Connected,
                this,
                static_cast<int>(id),
                -1
            )
            );
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            // Gamepad removed - we can close the underlying joystick if needed
            SDL_JoystickID id = ev->gdevice.which;
            CloseJoystick(id);
            SYSTEM_LOG << "Gamepad removed id=" << id << "\n";
            break;
		}
        case SDL_EVENT_JOYSTICK_REMOVED:
        {
            SDL_JoystickID id = ev->jdevice.which;
            CloseJoystick(id);
            SYSTEM_LOG << "Joystick removed id=" << id << "\n";
			// send notification to inputmanager
			short playerID = IM::Get().GetPlayerForController(id);
            IM::Get().UnbindControllerFromPlayer(playerID);
            IM::Get().AddDisconnectedPlayer(playerID, SDL_JoystickID(id));

			//Notify players that their controller has been disconnected
            EM::Get().DispatchImmediate
            (Message::Create
                (
                EventStructType::EventStructType_Olympe,
                EventType::Olympe_EventType_Joystick_Disconnected,
                this,
                static_cast<int>(id),
                -1
			    )
            );
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            SDL_JoystickID which = ev->gbutton.which;
            int button = ev->gbutton.button;
            bool down = ev->gbutton.down;
            PostJoystickButtonEvent(which, button, down);
            break;
		}
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        {
            SDL_JoystickID which = ev->jbutton.which;
            int button = ev->jbutton.button;
            bool down = ev->jbutton.down;
            
            // Update state for pull API
            if (button >= 0 && button < MAX_BUTTONS)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& state = m_joyStates[which];
                bool wasDown = state.buttons[button];
                state.buttons[button] = down;
                if (down && !wasDown)
                    state.buttonsPressed[button] = true;
                else if (!down && wasDown)
                    state.buttonsReleased[button] = true;
            }
            
            PostJoystickButtonEvent(which, button, down);
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            SDL_JoystickID which = ev->gbutton.which;
            int button = ev->gbutton.button;
            bool down = ev->gbutton.down;
            
            // Update state for pull API
            if (button >= 0 && button < MAX_BUTTONS)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& state = m_joyStates[which];
                bool wasDown = state.buttons[button];
                state.buttons[button] = down;
                if (down && !wasDown)
                    state.buttonsPressed[button] = true;
                else if (!down && wasDown)
                    state.buttonsReleased[button] = true;
            }
            
            PostJoystickButtonEvent(which, button, down);
            break;
        }
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        {
            SDL_JoystickID which = ev->jbutton.which;
            int button = ev->jbutton.button;
            bool down = ev->jbutton.down;
            
            // Update state for pull API
            if (button >= 0 && button < MAX_BUTTONS)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& state = m_joyStates[which];
                bool wasDown = state.buttons[button];
                state.buttons[button] = down;
                if (down && !wasDown)
                    state.buttonsPressed[button] = true;
                else if (!down && wasDown)
                    state.buttonsReleased[button] = true;
            }
            
            PostJoystickButtonEvent(which, button, down);
            break;
        }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            SDL_JoystickID which = ev->gaxis.which;
            int axis = ev->gaxis.axis;
            Sint16 value = ev->gaxis.value;
            
            // Update state for pull API (normalize to -1..1)
            if (axis >= 0 && axis < MAX_AXES)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& state = m_joyStates[which];
                float normalized = (value >= 0) ? (value / 32767.0f) : (value / 32768.0f);
                state.axes[axis] = normalized;
            }
            
            PostJoystickAxisEvent(which, axis, value);
			break;
		}
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        {
            SDL_JoystickID which = ev->jaxis.which;
            int axis = ev->jaxis.axis;
            Sint16 value = ev->jaxis.value;
            
            // Update state for pull API (normalize to -1..1)
            if (axis >= 0 && axis < MAX_AXES)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& state = m_joyStates[which];
                float normalized = (value >= 0) ? (value / 32767.0f) : (value / 32768.0f);
                state.axes[axis] = normalized;
            }
            
            PostJoystickAxisEvent(which, axis, value);
            break;
        }

        default:
            break;
    }
}
//---------------------------------------------------------------------------------------------
std::vector<SDL_JoystickID> JoystickManager::GetConnectedJoysticks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SDL_JoystickID> res;
    res.reserve(m_joysticks.size());
    for (auto &kv : m_joysticks) res.push_back(kv.first);
    return res;
}
//---------------------------------------------------------------------------------------------
bool JoystickManager::IsJoystickConnected(SDL_JoystickID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_joysticks.find(id) != m_joysticks.end();
}
//---------------------------------------------------------------------------------------------
void JoystickManager::OpenJoystick(SDL_JoystickID instance_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_joysticks.find(instance_id) != m_joysticks.end()) return; // already open

    SDL_Joystick* js = SDL_OpenJoystick(instance_id);
    if (!js)
    {
        SYSTEM_LOG << "Failed to open joystick " << instance_id << " : " << SDL_GetError() << "\n";
        return;
    }

    JoystickInfo info;
    info.joystick = js;
    info.id = SDL_GetJoystickID(js);
    const char* name = SDL_GetJoystickName(js);
    info.name = name ? name : "<unknown>";
    info.numAxes = SDL_GetNumJoystickAxes(js);
    info.numButtons = SDL_GetNumJoystickButtons(js);
    info.axes.assign(info.numAxes,0);
    info.buttons.assign(info.numButtons, false);

    // read initial states
    for (int a =0; a < info.numAxes; ++a)
    {
        info.axes[a] = SDL_GetJoystickAxis(js, a);
        float normalized = (info.axes[a] >= 0) ? (info.axes[a] / 32767.0f) : (info.axes[a] / 32768.0f);
        info.axes[a] = (Sint16) normalized;
    }
    for (int b =0; b < info.numButtons; ++b)
    {
        info.buttons[b] = SDL_GetJoystickButton(js, b);
    }

    m_joysticks[info.id] = std::move(info);
}
//---------------------------------------------------------------------------------------------
void JoystickManager::CloseJoystick(SDL_JoystickID instance_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_joysticks.find(instance_id);
    if (it == m_joysticks.end()) return;

    if (it->second.joystick)
    {
        SDL_CloseJoystick(it->second.joystick);
    }
    m_joysticks.erase(it);

    // Remove pull API state for this joystick
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_joyStates.erase(instance_id);
    }
}
//---------------------------------------------------------------------------------------------
void JoystickManager::PostJoystickButtonEvent(SDL_JoystickID which, int button, bool down)
{
    Message msg;
	msg.struct_type = EventStructType::EventStructType_Olympe;
    msg.msg_type = down ? EventType::Olympe_EventType_Joystick_ButtonDown : EventType::Olympe_EventType_Joystick_ButtonUp;
    msg.sender = this;
    msg.deviceId = static_cast<int>(which);
    msg.controlId = button;
    msg.state = down ?1 :0;
    msg.param1 = down ?1.0f :0.0f;

    EventManager::Get().AddMessage(msg);
}
//---------------------------------------------------------------------------------------------
void JoystickManager::PostJoystickAxisEvent(SDL_JoystickID which, int axis, Sint16 value)
{
    Message msg;
    msg.struct_type = EventStructType::EventStructType_Olympe;
    msg.msg_type = EventType::Olympe_EventType_Joystick_AxisMotion;
    msg.sender = this;
    msg.deviceId = static_cast<int>(which);
    msg.controlId = axis;
    msg.state =0;
    // normalize Sint16 (-32768..32767) to [-1.0,1.0]
    float normalized =0.0f;
    if (value >=0) normalized = (value /32767.0f);
    else normalized = (value /32768.0f);
    msg.param1 = normalized;

    EventManager::Get().AddMessage(msg);
}
//---------------------------------------------------------------------------------------------
void JoystickManager::PostJoystickConnectedEvent(SDL_JoystickID which, bool bconnected)
{
	Message msg;
    msg.struct_type = EventStructType::EventStructType_Olympe;
    msg.msg_type = bconnected ? EventType::Olympe_EventType_Joystick_Connected : EventType::Olympe_EventType_Joystick_Disconnected;
    msg.sender = this;
    msg.deviceId = static_cast<int>(which);
	EventManager::Get().AddMessage(msg);
}
