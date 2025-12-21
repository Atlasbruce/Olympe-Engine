#include "InputsManager.h"
#include "system/message.h"
#include <algorithm>

InputsManager::InputsManager()
{
    name = "InputsManager";

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