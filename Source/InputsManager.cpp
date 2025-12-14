#include "InputsManager.h"
#include "system/message.h"
#include "system/eventmanager.h"
#include <algorithm>

using EM = ::EventManager;

InputsManager::InputsManager()
{
    name = "InputsManager";

	EM::Get().Register(this, EventType::Olympe_EventType_Joystick_Disconnected);
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_Connected);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_Connected);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_Disconnected);


    SYSTEM_LOG << "InputsManager created and Initialized\n";
}

InputsManager::~InputsManager()
{
    Shutdown();
    SYSTEM_LOG << "InputsManager destroyed\n";
}

void InputsManager::HandleEvent(const SDL_Event* ev)
{
    JoystickManager::Get().HandleEvent(ev);
    KeyboardManager::Get().HandleEvent(ev);
    MouseManager::Get().HandleEvent(ev);
}

void InputsManager::OnEvent(const Message& msg)
{
    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Joystick_Connected:
        {
			// Auto reconnect joystick to any player that was disconnected if any
            if (GetDisconnectedPlayersCount() > 0)
            {
				// get 1st Disconnected player
				short disconnectedPlayerID = GetFirstDisconnectedPlayerID();
				if (disconnectedPlayerID >= 0)
                {
                    SYSTEM_LOG << "InputsManager: try rebinding joystick ID=" << msg.deviceId << " to disconnected player " << disconnectedPlayerID << "\n";
                    //BindControllerToPlayer(disconnectedPlayerID, msg.deviceId);
                    if (AutoBindControllerToPlayer(disconnectedPlayerID))
                    {
						// we can remove the disconnected player now, since he is rebound
						RemoveDisconnectedPlayer(disconnectedPlayerID);
						SYSTEM_LOG << "InputsManager: Joystick ID=" << msg.deviceId << " rebound to player " << disconnectedPlayerID << "\n";
                    }
                    else
						SYSTEM_LOG << "InputsManager: Failed to rebind joystick ID=" << msg.deviceId << " to disconnected player " << disconnectedPlayerID << "\n";
                }
			}
            break;
		}
    }
}
//-------------------------------------------------------------
// set a strin with the status and info of all connected devices (joysticks, keyboard and mouse)
// state of connectivity, bouds to player ID etc...
// the returned string is stored internally and updated at each call and will be use by the PanelManager InputsInspector panel
string InputsManager::GetDevicesStatusUpdate()
{
    m_devicesStatus.str(std::string());
    m_devicesStatus << "---- InputsManager Devices Status ----\r\n";
    // Joysticks
    auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
    m_devicesStatus << "Connected Joysticks: " << joysticks.size() << "\r\n";
	m_devicesStatus << "Available Unassigned Joysticks: " << GetAvailableJoystickCount() << "\r\n";
	m_devicesStatus << "Available Assigned Joysticks: " << (GetConnectedJoysticksCount() - GetAvailableJoystickCount()) << "\r\n";
    for (auto jid : joysticks)
    {
        m_devicesStatus << "  - Joystick ID=" << jid;
        // find which player is bound to this joystick
        short boundPlayerID = -1;
        for (auto& kv : m_playerBindings)
        {
            if (kv.second == jid) { boundPlayerID = kv.first; break; }
        }
        if (boundPlayerID >= 0)
            m_devicesStatus << "  -> Bound to Player " << boundPlayerID << "\r\n";
        else
            m_devicesStatus << "  -> Not bound to any player\r\n";
    }
    // Keyboard
    m_devicesStatus << "Keyboard: ";
    if (m_keyboardAssigned)
    {
        m_devicesStatus << "Assigned to Player ";
        // find which player is bound to keyboard
        short boundPlayerID = -1;
        for (auto& kv : m_playerBindings)
        {
            if (kv.second == SDL_JoystickID(-1)) { boundPlayerID = kv.first; break; }
        }
        if (boundPlayerID >= 0)
            m_devicesStatus << boundPlayerID << "\r\n";
        else
            m_devicesStatus << "(error: assigned but no player?)\r\n";
    }
    else
    {
        m_devicesStatus << "Not assigned\r\n";
    }
    // Mouse
    m_devicesStatus << "Mouse: Connected\r\n"; // assume always connected for now
    return m_devicesStatus.str();
}
//-------------------------------------------------------------
// Input Context Stack
void InputsManager::PushContext(InputContext ctx)
{
    m_contextStack.push_back(ctx);
    SYSTEM_LOG << "InputsManager: Pushed context " << static_cast<int>(ctx) << ", stack size: " << m_contextStack.size() << "\n";
}

void InputsManager::PopContext()
{
    if (m_contextStack.size() > 1) // Keep at least one context
    {
        m_contextStack.pop_back();
        SYSTEM_LOG << "InputsManager: Popped context, stack size: " << m_contextStack.size() << "\n";
    }
    else
    {
        SYSTEM_LOG << "InputsManager: Cannot pop last context (stack would be empty)\n";
    }
}

InputContext InputsManager::GetActiveContext() const
{
    return m_contextStack.back();
}
//-------------------------------------------------------------
// Input Entity Cache
void InputsManager::RegisterInputEntity(EntityID e)
{
    // Check if already registered
    for (EntityID existing : m_inputEntities)
    {
        if (existing == e) return;
    }
    m_inputEntities.push_back(e);
    SYSTEM_LOG << "InputsManager: Registered input entity " << e << "\n";
}

void InputsManager::UnregisterInputEntity(EntityID e)
{
    auto it = std::find(m_inputEntities.begin(), m_inputEntities.end(), e);
    if (it != m_inputEntities.end())
    {
        m_inputEntities.erase(it);
        SYSTEM_LOG << "InputsManager: Unregistered input entity " << e << "\n";
    }
}

const std::vector<EntityID>& InputsManager::GetInputEntities() const
{
    return m_inputEntities;
}