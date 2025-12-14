# 💡 Input System - Exemples Pratiques

> **Collection d'exemples concrets pour utiliser le système Input ECS.**

---

## Table des matières

- [Exemple 1 : Déplacement 4 directions (Keyboard)](#exemple-1--déplacement-4-directions-keyboard)
- [Exemple 2 : Déplacement analogique (Gamepad)](#exemple-2--déplacement-analogique-gamepad)
- [Exemple 3 : Multi-joueur local (2 joueurs)](#exemple-3--multi-joueur-local-2-joueurs)
- [Exemple 4 : Gestion du hotplug](#exemple-4--gestion-du-hotplug)
- [Exemple 5 : Listener d'événements input](#exemple-5--listener-dévénements-input)
- [Exemple 6 : Système de tir avec cooldown](#exemple-6--système-de-tir-avec-cooldown)
- [Exemple 7 : Dash/Sprint avec stamina](#exemple-7--dashsprint-avec-stamina)

---

## Exemple 1 : Déplacement 4 directions (Keyboard)

Créer un joueur contrôlable au clavier avec déplacement 4 directions (WASD ou flèches).

### Code complet

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "InputsManager.h"
#include "system/KeyboardManager.h"
#include "system/EventManager.h"

// ========== 1. Créer l'entité joueur ==========

EntityID CreateKeyboardPlayer()
{
    EntityID player = World::Get().CreateEntity();
    
    // Position de départ
    Position_data pos;
    pos.position = Vector{400.f, 300.f, 0.f};
    World::Get().AddComponent<Position_data>(player, pos);
    
    // Binding au clavier
    PlayerBinding_data binding;
    binding.playerIndex = 0;
    binding.controllerID = -1; // Keyboard
    World::Get().AddComponent<PlayerBinding_data>(player, binding);
    
    // Controller state
    Controller_data ctrl;
    ctrl.controllerID = -1;
    ctrl.isConnected = true;
    World::Get().AddComponent<Controller_data>(player, ctrl);
    
    // Player controller
    World::Get().AddComponent<PlayerController_data>(player);
    
    // Visual (optional)
    BoundingBox_data bbox;
    bbox.boundingBox = {0, 0, 32, 32};
    World::Get().AddComponent<BoundingBox_data>(player, bbox);
    
    // Bind dans InputsManager
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
    
    std::cout << "✅ Keyboard player created (Entity " << player << ")" << std::endl;
    
    return player;
}

// ========== 2. Système de mapping clavier → gameplay ==========

class KeyboardInputSystem : public ECS_System
{
public:
    KeyboardInputSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
    }
    
    void Process() override
    {
        // Get keyboard state (via SDL)
        const bool* keyState = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            
            // Only process keyboard-bound entities
            if (binding.controllerID != -1)
                continue;
            
            // Reset direction
            pc.Joydirection = Vector{0.f, 0.f, 0.f};
            
            // WASD or Arrow keys
            if (keyState[SDL_SCANCODE_W] || keyState[SDL_SCANCODE_UP])
                pc.Joydirection.y = -1.f; // Up
            if (keyState[SDL_SCANCODE_S] || keyState[SDL_SCANCODE_DOWN])
                pc.Joydirection.y = 1.f;  // Down
            if (keyState[SDL_SCANCODE_A] || keyState[SDL_SCANCODE_LEFT])
                pc.Joydirection.x = -1.f; // Left
            if (keyState[SDL_SCANCODE_D] || keyState[SDL_SCANCODE_RIGHT])
                pc.Joydirection.x = 1.f;  // Right
            
            // Normalize diagonal movement
            float length = std::sqrt(pc.Joydirection.x * pc.Joydirection.x + 
                                     pc.Joydirection.y * pc.Joydirection.y);
            if (length > 1.f)
            {
                pc.Joydirection.x /= length;
                pc.Joydirection.y /= length;
            }
            
            // Actions
            pc.isJumping = keyState[SDL_SCANCODE_SPACE];
            pc.isShooting = keyState[SDL_SCANCODE_LCTRL] || keyState[SDL_SCANCODE_RCTRL];
            pc.isInteracting = keyState[SDL_SCANCODE_E];
        }
    }
};

// ========== 3. Système de mouvement ==========

class SimpleMovementSystem : public ECS_System
{
public:
    SimpleMovementSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
    }
    
    void Process() override
    {
        float speed = 200.f; // pixels per second
        
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            // Apply movement
            pos.position.x += pc.Joydirection.x * speed * GameEngine::fDt;
            pos.position.y += pc.Joydirection.y * speed * GameEngine::fDt;
        }
    }
};

// ========== 4. Utilisation ==========

void SetupKeyboardPlayer()
{
    // Create systems
    auto inputSystem = std::make_shared<KeyboardInputSystem>();
    auto movementSystem = std::make_shared<SimpleMovementSystem>();
    
    // Register systems
    World::Get().RegisterSystem(inputSystem);
    World::Get().RegisterSystem(movementSystem);
    
    // Create player entity
    EntityID player = CreateKeyboardPlayer();
    
    std::cout << "✅ Keyboard player setup complete!" << std::endl;
}
```

### Résultat

- **W/↑** : déplacement vers le haut
- **S/↓** : déplacement vers le bas
- **A/←** : déplacement vers la gauche
- **D/→** : déplacement vers la droite
- **Space** : saut
- **Ctrl** : tir
- **E** : interaction

---

## Exemple 2 : Déplacement analogique (Gamepad)

Utiliser les sticks analogiques d'une manette avec gestion de deadzone.

### Code complet

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "InputsManager.h"
#include "system/JoystickManager.h"
#include <SDL3/SDL.h>

// ========== 1. Créer l'entité joueur gamepad ==========

EntityID CreateGamepadPlayer(SDL_JoystickID joystickID)
{
    EntityID player = World::Get().CreateEntity();
    
    // Position
    Position_data pos;
    pos.position = Vector{400.f, 300.f, 0.f};
    World::Get().AddComponent<Position_data>(player, pos);
    
    // Binding au gamepad
    PlayerBinding_data binding;
    binding.playerIndex = 1; // Player 2
    binding.controllerID = joystickID;
    World::Get().AddComponent<PlayerBinding_data>(player, binding);
    
    // Controller state
    Controller_data ctrl;
    ctrl.controllerID = joystickID;
    ctrl.isConnected = true;
    World::Get().AddComponent<Controller_data>(player, ctrl);
    
    // Player controller
    World::Get().AddComponent<PlayerController_data>(player);
    
    // Visual
    BoundingBox_data bbox;
    bbox.boundingBox = {0, 0, 32, 32};
    World::Get().AddComponent<BoundingBox_data>(player, bbox);
    
    // Bind dans InputsManager
    InputsManager::Get().BindControllerToPlayer(1, joystickID);
    
    std::cout << "✅ Gamepad player created (Entity " << player << ", Joystick " << joystickID << ")" << std::endl;
    
    return player;
}

// ========== 2. Système de mapping gamepad → gameplay ==========

class GamepadInputSystem : public ECS_System
{
public:
    GamepadInputSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Controller_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
            
            // Only process gamepad-bound entities
            if (binding.controllerID < 0 || !ctrl.isConnected)
                continue;
            
            SDL_JoystickID jid = static_cast<SDL_JoystickID>(binding.controllerID);
            
            // Check if joystick is still connected
            if (!JoystickManager::Get().IsJoystickConnected(jid))
            {
                ctrl.isConnected = false;
                continue;
            }
            
            // Get SDL joystick handle (internal access needed)
            // For now, we'll simulate with direct SDL calls
            SDL_Joystick* joy = SDL_GetJoystickFromID(jid);
            if (!joy)
                continue;
            
            // Read left stick (axis 0 = X, axis 1 = Y)
            Sint16 axisX = SDL_GetJoystickAxis(joy, 0);
            Sint16 axisY = SDL_GetJoystickAxis(joy, 1);
            
            // Convert Sint16 (-32768 to 32767) to float (-1.0 to 1.0)
            float x = axisX / 32767.f;
            float y = axisY / 32767.f;
            
            // Apply deadzone (0.15 = 15%)
            const float deadzone = 0.15f;
            if (std::abs(x) < deadzone) x = 0.f;
            if (std::abs(y) < deadzone) y = 0.f;
            
            // Normalize outside deadzone
            if (x != 0.f || y != 0.f)
            {
                float length = std::sqrt(x * x + y * y);
                if (length > 1.f)
                {
                    x /= length;
                    y /= length;
                }
            }
            
            pc.Joydirection.x = x;
            pc.Joydirection.y = y;
            
            // Read buttons
            pc.isJumping = SDL_GetJoystickButton(joy, 0);  // Button A (Xbox)
            pc.isShooting = SDL_GetJoystickButton(joy, 2); // Button X (Xbox)
            pc.isInteracting = SDL_GetJoystickButton(joy, 1); // Button B (Xbox)
        }
    }
};

// ========== 3. Utilisation ==========

void SetupGamepadPlayer()
{
    // Check for connected joysticks
    auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
    if (joysticks.empty())
    {
        std::cerr << "❌ No joystick connected!" << std::endl;
        return;
    }
    
    SDL_JoystickID jid = joysticks[0];
    
    // Create system
    auto inputSystem = std::make_shared<GamepadInputSystem>();
    World::Get().RegisterSystem(inputSystem);
    
    // Create player entity
    EntityID player = CreateGamepadPlayer(jid);
    
    std::cout << "✅ Gamepad player setup complete!" << std::endl;
}
```

### Deadzone expliquée

```
Stick analogique brut (Sint16):
    -32768 ←──────── 0 ────────→ 32767
         Left              Right

Converti en float:
    -1.0 ←──────── 0.0 ────────→ 1.0

Avec deadzone 0.15:
    [-1.0 ... -0.15] → input valide
    [-0.15 ... 0.15] → force à 0.0 (zone morte)
    [0.15 ... 1.0]   → input valide
```

**Pourquoi une deadzone ?**
- Évite les micro-mouvements dus aux sticks imprécis
- Rend le jeu plus confortable (pas de "drift")

---

## Exemple 3 : Multi-joueur local (2 joueurs)

Setup complet avec joueur 1 au clavier et joueur 2 à la manette.

### Code complet

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "InputsManager.h"

// ========== Setup multi-joueur ==========

struct LocalMultiplayerSetup
{
    EntityID player1; // Keyboard
    EntityID player2; // Gamepad
};

LocalMultiplayerSetup CreateLocalMultiplayer()
{
    LocalMultiplayerSetup setup;
    
    // ========== Player 1: Keyboard ==========
    
    setup.player1 = World::Get().CreateEntity();
    
    Position_data pos1;
    pos1.position = Vector{200.f, 300.f, 0.f};
    World::Get().AddComponent<Position_data>(setup.player1, pos1);
    
    PlayerBinding_data binding1;
    binding1.playerIndex = 0;
    binding1.controllerID = -1; // Keyboard
    World::Get().AddComponent<PlayerBinding_data>(setup.player1, binding1);
    
    Controller_data ctrl1;
    ctrl1.controllerID = -1;
    ctrl1.isConnected = true;
    World::Get().AddComponent<Controller_data>(setup.player1, ctrl1);
    
    World::Get().AddComponent<PlayerController_data>(setup.player1);
    
    BoundingBox_data bbox1;
    bbox1.boundingBox = {0, 0, 32, 32};
    World::Get().AddComponent<BoundingBox_data>(setup.player1, bbox1);
    
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
    
    std::cout << "✅ Player 1 (Keyboard) created" << std::endl;
    
    // ========== Player 2: Gamepad ==========
    
    auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
    if (!joysticks.empty())
    {
        SDL_JoystickID jid = joysticks[0];
        
        setup.player2 = World::Get().CreateEntity();
        
        Position_data pos2;
        pos2.position = Vector{600.f, 300.f, 0.f};
        World::Get().AddComponent<Position_data>(setup.player2, pos2);
        
        PlayerBinding_data binding2;
        binding2.playerIndex = 1;
        binding2.controllerID = jid;
        World::Get().AddComponent<PlayerBinding_data>(setup.player2, binding2);
        
        Controller_data ctrl2;
        ctrl2.controllerID = jid;
        ctrl2.isConnected = true;
        World::Get().AddComponent<Controller_data>(setup.player2, ctrl2);
        
        World::Get().AddComponent<PlayerController_data>(setup.player2);
        
        BoundingBox_data bbox2;
        bbox2.boundingBox = {0, 0, 32, 32};
        World::Get().AddComponent<BoundingBox_data>(setup.player2, bbox2);
        
        InputsManager::Get().BindControllerToPlayer(1, jid);
        
        std::cout << "✅ Player 2 (Joystick " << jid << ") created" << std::endl;
    }
    else
    {
        std::cerr << "⚠️ No gamepad found for Player 2" << std::endl;
        setup.player2 = INVALID_ENTITY_ID;
    }
    
    return setup;
}

// ========== Système multi-joueur unifié ==========

class MultiplayerInputSystem : public ECS_System
{
public:
    MultiplayerInputSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Controller_data>(), true);
    }
    
    void Process() override
    {
        const bool* keyState = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
            
            // Reset
            pc.Joydirection = Vector{0.f, 0.f, 0.f};
            pc.isJumping = false;
            pc.isShooting = false;
            
            // ========== Keyboard player ==========
            if (binding.controllerID == -1)
            {
                if (keyState[SDL_SCANCODE_W]) pc.Joydirection.y = -1.f;
                if (keyState[SDL_SCANCODE_S]) pc.Joydirection.y = 1.f;
                if (keyState[SDL_SCANCODE_A]) pc.Joydirection.x = -1.f;
                if (keyState[SDL_SCANCODE_D]) pc.Joydirection.x = 1.f;
                
                pc.isJumping = keyState[SDL_SCANCODE_SPACE];
                pc.isShooting = keyState[SDL_SCANCODE_LCTRL];
            }
            // ========== Gamepad player ==========
            else if (ctrl.isConnected)
            {
                SDL_Joystick* joy = SDL_GetJoystickFromID(static_cast<SDL_JoystickID>(binding.controllerID));
                if (joy)
                {
                    float x = SDL_GetJoystickAxis(joy, 0) / 32767.f;
                    float y = SDL_GetJoystickAxis(joy, 1) / 32767.f;
                    
                    const float deadzone = 0.15f;
                    if (std::abs(x) < deadzone) x = 0.f;
                    if (std::abs(y) < deadzone) y = 0.f;
                    
                    pc.Joydirection.x = x;
                    pc.Joydirection.y = y;
                    
                    pc.isJumping = SDL_GetJoystickButton(joy, 0);
                    pc.isShooting = SDL_GetJoystickButton(joy, 2);
                }
            }
            
            // Normalize diagonal
            float len = std::sqrt(pc.Joydirection.x * pc.Joydirection.x + 
                                  pc.Joydirection.y * pc.Joydirection.y);
            if (len > 1.f)
            {
                pc.Joydirection.x /= len;
                pc.Joydirection.y /= len;
            }
        }
    }
};

// ========== Utilisation ==========

void SetupLocalMultiplayer()
{
    // Create input system
    auto inputSystem = std::make_shared<MultiplayerInputSystem>();
    World::Get().RegisterSystem(inputSystem);
    
    // Create movement system (from previous examples)
    auto movementSystem = std::make_shared<SimpleMovementSystem>();
    World::Get().RegisterSystem(movementSystem);
    
    // Create players
    LocalMultiplayerSetup setup = CreateLocalMultiplayer();
    
    std::cout << "✅ Local multiplayer setup complete!" << std::endl;
    std::cout << "   Player 1 (Keyboard): Entity " << setup.player1 << std::endl;
    if (setup.player2 != INVALID_ENTITY_ID)
        std::cout << "   Player 2 (Gamepad): Entity " << setup.player2 << std::endl;
}
```

---

## Exemple 4 : Gestion du hotplug

Détecter et gérer automatiquement la déconnexion/reconnexion des manettes.

### Code complet

```cpp
#include "InputsManager.h"
#include "system/EventManager.h"
#include "system/message.h"

// ========== Listener pour les événements de connexion ==========

class HotplugListener : public Object
{
public:
    HotplugListener()
    {
        name = "HotplugListener";
        
        // S'abonner aux événements
        EventManager::Get().Register(this, EventType::Olympe_EventType_Joystick_Connected);
        EventManager::Get().Register(this, EventType::Olympe_EventType_Joystick_Disconnected);
    }
    
    void OnEvent(const Message& msg) override
    {
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Joystick_Connected:
            {
                SDL_JoystickID jid = static_cast<SDL_JoystickID>(msg.deviceId);
                std::cout << "🎮 Joystick CONNECTED: ID " << jid << std::endl;
                
                // Check if we can auto-rebind a disconnected player
                if (InputsManager::Get().GetDisconnectedPlayersCount() > 0)
                {
                    short playerID = InputsManager::Get().GetFirstDisconnectedPlayerID();
                    std::cout << "   → Attempting to rebind to Player " << playerID << std::endl;
                    
                    if (InputsManager::Get().AutoBindControllerToPlayer(playerID))
                    {
                        InputsManager::Get().RemoveDisconnectedPlayer(playerID);
                        std::cout << "   ✅ Successfully rebound to Player " << playerID << std::endl;
                        
                        // Update entity Controller_data
                        // (would need access to player entities here)
                    }
                }
                else
                {
                    std::cout << "   ℹ️ No disconnected players to rebind" << std::endl;
                }
                break;
            }
            
            case EventType::Olympe_EventType_Joystick_Disconnected:
            {
                SDL_JoystickID jid = static_cast<SDL_JoystickID>(msg.deviceId);
                std::cout << "🎮 Joystick DISCONNECTED: ID " << jid << std::endl;
                
                // Find which player was using this joystick
                short playerID = InputsManager::Get().GetPlayerForController(jid);
                if (playerID >= 0)
                {
                    std::cout << "   ⚠️ Player " << playerID << " has lost their controller!" << std::endl;
                    
                    // Mark player as disconnected
                    InputsManager::Get().AddDisconnectedPlayer(playerID, jid);
                    InputsManager::Get().UnbindControllerFromPlayer(playerID);
                    
                    // Update entity Controller_data to mark as disconnected
                    // (would need access to player entities here)
                }
                break;
            }
        }
    }
};

// ========== Système pour gérer les joueurs déconnectés ==========

class DisconnectedPlayerSystem : public ECS_System
{
public:
    DisconnectedPlayerSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Controller_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            
            // Check if joystick is still connected (for gamepad players)
            if (binding.controllerID >= 0)
            {
                bool connected = JoystickManager::Get().IsJoystickConnected(
                    static_cast<SDL_JoystickID>(binding.controllerID));
                
                if (!connected && ctrl.isConnected)
                {
                    // Just disconnected
                    ctrl.isConnected = false;
                    std::cout << "⚠️ Entity " << entity << " controller disconnected" << std::endl;
                }
                else if (connected && !ctrl.isConnected)
                {
                    // Just reconnected
                    ctrl.isConnected = true;
                    std::cout << "✅ Entity " << entity << " controller reconnected" << std::endl;
                }
            }
        }
    }
};

// ========== Utilisation ==========

void SetupHotplugHandling()
{
    // Create listener
    HotplugListener* listener = new HotplugListener();
    
    // Create system
    auto disconnectedSystem = std::make_shared<DisconnectedPlayerSystem>();
    World::Get().RegisterSystem(disconnectedSystem);
    
    std::cout << "✅ Hotplug handling setup complete!" << std::endl;
}
```

### Comportement

1. **Déconnexion** :
   - Événement `Joystick_Disconnected` détecté
   - Le joueur est marqué comme "disconnected"
   - Le binding est retiré
   - L'entité du joueur est désactivée (ou pause)

2. **Reconnexion** :
   - Événement `Joystick_Connected` détecté
   - Si un joueur était déconnecté, tentative de rebind automatique
   - L'entité du joueur est réactivée

---

## Exemple 5 : Listener d'événements input

Écouter les événements clavier/manette pour des actions spécifiques.

### Code complet

```cpp
#include "system/EventManager.h"
#include "system/message.h"

// ========== Listener pour les actions de jeu ==========

class GameActionsListener : public Object
{
public:
    GameActionsListener()
    {
        name = "GameActionsListener";
        
        // S'abonner aux événements
        EventManager::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyDown);
        EventManager::Get().Register(this, EventType::Olympe_EventType_Joystick_ButtonDown);
    }
    
    void OnEvent(const Message& msg) override
    {
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Keyboard_KeyDown:
            {
                SDL_Scancode key = static_cast<SDL_Scancode>(msg.controlId);
                
                // Pause game (ESC)
                if (key == SDL_SCANCODE_ESCAPE)
                {
                    std::cout << "⏸️ Pause game!" << std::endl;
                    // GameState::Get().SetPaused(true);
                }
                
                // Open inventory (TAB)
                if (key == SDL_SCANCODE_TAB)
                {
                    std::cout << "🎒 Open inventory!" << std::endl;
                    // InventoryUI::Get().Open();
                }
                
                // Quick save (F5)
                if (key == SDL_SCANCODE_F5)
                {
                    std::cout << "💾 Quick save!" << std::endl;
                    // SaveSystem::Get().QuickSave();
                }
                
                break;
            }
            
            case EventType::Olympe_EventType_Joystick_ButtonDown:
            {
                SDL_JoystickID jid = static_cast<SDL_JoystickID>(msg.deviceId);
                int button = msg.controlId;
                
                // Determine which player pressed the button
                short playerID = InputsManager::Get().GetPlayerForController(jid);
                
                std::cout << "🎮 Player " << playerID << " pressed button " << button << std::endl;
                
                // Start button (pause)
                if (button == 7) // Start button on most controllers
                {
                    std::cout << "⏸️ Player " << playerID << " paused the game!" << std::endl;
                }
                
                break;
            }
        }
    }
};

// ========== Utilisation ==========

void SetupGameActionsListener()
{
    GameActionsListener* listener = new GameActionsListener();
    std::cout << "✅ Game actions listener setup complete!" << std::endl;
}
```

---

## Exemple 6 : Système de tir avec cooldown

Empêcher le spam de tir avec un cooldown.

### Code complet

```cpp
#include "ECS_Components.h"
#include "ECS_Systems.h"

// ========== Component pour le cooldown ==========

struct WeaponCooldown_data
{
    float cooldownTime = 0.2f;    // 0.2 secondes entre chaque tir
    float timeSinceLastShot = 0.f; // Temps écoulé depuis le dernier tir
};

// ========== Système de tir ==========

class ShootingSystem : public ECS_System
{
public:
    ShootingSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<WeaponCooldown_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            WeaponCooldown_data& weapon = World::Get().GetComponent<WeaponCooldown_data>(entity);
            
            // Update cooldown timer
            weapon.timeSinceLastShot += GameEngine::fDt;
            
            // Check if player is shooting and cooldown expired
            if (pc.isShooting && weapon.timeSinceLastShot >= weapon.cooldownTime)
            {
                // Fire projectile
                FireProjectile(pos.position, pc.Joydirection);
                
                // Reset cooldown
                weapon.timeSinceLastShot = 0.f;
                
                std::cout << "💥 Entity " << entity << " fired!" << std::endl;
            }
        }
    }
    
private:
    void FireProjectile(const Vector& position, const Vector& direction)
    {
        // Create projectile entity
        EntityID projectile = World::Get().CreateEntity();
        
        Position_data projPos;
        projPos.position = position;
        World::Get().AddComponent<Position_data>(projectile, projPos);
        
        Movement_data projMove;
        projMove.direction = direction.Normalized();
        projMove.velocity = Vector{500.f, 0.f, 0.f}; // Fast projectile
        World::Get().AddComponent<Movement_data>(projectile, projMove);
        
        // Add visual, collision, etc.
    }
};

// ========== Utilisation ==========

void SetupShootingSystem()
{
    auto shootingSystem = std::make_shared<ShootingSystem>();
    World::Get().RegisterSystem(shootingSystem);
    
    // Add weapon cooldown to player entities
    // (assuming player1, player2 exist)
    WeaponCooldown_data weapon;
    weapon.cooldownTime = 0.2f;
    World::Get().AddComponent<WeaponCooldown_data>(player1, weapon);
    
    std::cout << "✅ Shooting system setup complete!" << std::endl;
}
```

---

## Exemple 7 : Dash/Sprint avec stamina

Système de sprint avec consommation de stamina et cooldown.

### Code complet

```cpp
#include "ECS_Components.h"
#include "ECS_Systems.h"

// ========== Component pour stamina ==========

struct Stamina_data
{
    float maxStamina = 100.f;
    float currentStamina = 100.f;
    float staminaRegenRate = 20.f;    // par seconde
    float sprintCost = 30.f;          // par seconde
    float dashCost = 25.f;            // coût fixe pour un dash
    float dashCooldown = 1.0f;        // cooldown entre 2 dashs
    float timeSinceLastDash = 0.f;
};

// ========== Système de sprint/dash ==========

class SprintDashSystem : public ECS_System
{
public:
    SprintDashSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Stamina_data>(), true);
    }
    
    void Process() override
    {
        const bool* keyState = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Stamina_data& stamina = World::Get().GetComponent<Stamina_data>(entity);
            
            float dt = GameEngine::fDt;
            
            // Update dash cooldown
            stamina.timeSinceLastDash += dt;
            
            // ========== Dash (Shift key) ==========
            bool dashPressed = keyState[SDL_SCANCODE_LSHIFT];
            
            if (dashPressed && 
                stamina.timeSinceLastDash >= stamina.dashCooldown &&
                stamina.currentStamina >= stamina.dashCost)
            {
                // Perform dash
                Vector dashDir = pc.Joydirection;
                if (dashDir.Length() > 0.1f)
                {
                    dashDir = dashDir.Normalized();
                    float dashDistance = 100.f; // pixels
                    
                    pos.position += dashDir * dashDistance;
                    
                    // Consume stamina
                    stamina.currentStamina -= stamina.dashCost;
                    stamina.timeSinceLastDash = 0.f;
                    
                    std::cout << "💨 Entity " << entity << " dashed!" << std::endl;
                }
            }
            
            // ========== Sprint (hold running) ==========
            if (pc.isRunning && stamina.currentStamina > 0.f)
            {
                // Consume stamina while sprinting
                stamina.currentStamina -= stamina.sprintCost * dt;
                if (stamina.currentStamina < 0.f)
                    stamina.currentStamina = 0.f;
            }
            else
            {
                // Regenerate stamina
                stamina.currentStamina += stamina.staminaRegenRate * dt;
                if (stamina.currentStamina > stamina.maxStamina)
                    stamina.currentStamina = stamina.maxStamina;
            }
        }
    }
};

// ========== Système de mouvement avec sprint ==========

class SprintMovementSystem : public ECS_System
{
public:
    SprintMovementSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Stamina_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            Stamina_data& stamina = World::Get().GetComponent<Stamina_data>(entity);
            
            float baseSpeed = 150.f;
            float sprintMultiplier = 2.0f;
            
            // Use sprint speed if running and has stamina
            float speed = baseSpeed;
            if (pc.isRunning && stamina.currentStamina > 0.f)
            {
                speed = baseSpeed * sprintMultiplier;
            }
            
            // Apply movement
            pos.position += pc.Joydirection * speed * GameEngine::fDt;
        }
    }
};

// ========== Utilisation ==========

void SetupSprintDashSystem()
{
    auto sprintDashSystem = std::make_shared<SprintDashSystem>();
    auto movementSystem = std::make_shared<SprintMovementSystem>();
    
    World::Get().RegisterSystem(sprintDashSystem);
    World::Get().RegisterSystem(movementSystem);
    
    // Add stamina to player entities
    Stamina_data stamina;
    World::Get().AddComponent<Stamina_data>(player1, stamina);
    
    std::cout << "✅ Sprint/Dash system setup complete!" << std::endl;
}
```

---

## Résumé des exemples

| Exemple | Difficulté | Concepts clés |
|---------|-----------|---------------|
| 1. Déplacement 4 directions | ⭐ Facile | Keyboard input, normalization |
| 2. Déplacement analogique | ⭐⭐ Moyen | Gamepad, deadzone, analog sticks |
| 3. Multi-joueur local | ⭐⭐ Moyen | Multiple players, mixed inputs |
| 4. Hotplug | ⭐⭐⭐ Avancé | Event listeners, reconnection |
| 5. Listener d'événements | ⭐⭐ Moyen | Event system, message passing |
| 6. Tir avec cooldown | ⭐⭐ Moyen | Cooldown timer, projectiles |
| 7. Sprint/Dash | ⭐⭐⭐ Avancé | Stamina system, multiple mechanics |

---

**Prochaines étapes :**
- 🏗️ [Architecture](INPUT_ARCHITECTURE.md) - Comprendre les décisions de design
- 🔧 [API Reference](INPUT_API_REFERENCE.md) - Documentation complète
- 🐛 [Troubleshooting](INPUT_TROUBLESHOOTING.md) - Résoudre les problèmes

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
