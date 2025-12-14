#pragma once

#include "../object.h"
#include "EventManager.h"
#include "message.h"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

class JoystickManager : public Object
{
public:
    JoystickManager()
    {
        name = "JoystickManager";
		Initialize();
    }
    virtual ~JoystickManager()
    {
		Shutdown();
	}

    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }

    static JoystickManager& GetInstance();
    static JoystickManager& Get() { return GetInstance(); }

    // Initialize joystick subsystem and open currently connected devices
    void Initialize();
    void Scan_Joysticks();
    void Shutdown();

    // Process per-frame (optional)
    void Process(float dt);

    // Handle an incoming SDL_Event (forwarded from the application event pump)
    void HandleEvent(const SDL_Event* ev);

    // Query
    std::vector<SDL_JoystickID> GetConnectedJoysticks();
    bool IsJoystickConnected(SDL_JoystickID id);

    // Pull API for reading joystick state
    void BeginFrame();
    bool GetButton(SDL_JoystickID id, int button) const;
    bool IsButtonPressed(SDL_JoystickID id, int button) const;
    bool IsButtonReleased(SDL_JoystickID id, int button) const;
    float GetAxis(SDL_JoystickID id, int axis) const;

private:
    struct JoystickInfo
    {
        SDL_Joystick* joystick = nullptr;
        SDL_JoystickID id = 0;
        std::string name;
        int numAxes = 0;
        int numButtons = 0;
        std::vector<Sint16> axes;
        std::vector<bool> buttons;
    };

    // State tracking for pull API
    struct JoystickState
    {
        bool connected = false;
        float axes[6] = {0};
        bool buttons[16] = {false};
        bool buttonsPressed[16] = {false};
        bool buttonsReleased[16] = {false};
    };

    std::unordered_map<SDL_JoystickID, JoystickInfo> m_joysticks;
    std::unordered_map<SDL_JoystickID, JoystickState> m_joyStates;
    std::mutex m_mutex;

    void OpenJoystick(SDL_JoystickID instance_id);
    void CloseJoystick(SDL_JoystickID instance_id);
    void PostJoystickButtonEvent(SDL_JoystickID which, int button, bool down);
    void PostJoystickAxisEvent(SDL_JoystickID which, int axis, Sint16 value);
    void PostJoystickConnectedEvent(SDL_JoystickID which, bool bconnected);
};
