# 📖 Input System Guide - Olympe Engine

> **Version:** 2.0 (2025)  
> **SDL Version:** SDL3  
> **Status:** Phase 1 - Production Ready

---

## Table of Contents

- [Vue d'ensemble](#vue-densemble)
- [Concepts clés](#concepts-clés)
- [Architecture](#architecture)
- [Pipeline d'exécution](#pipeline-dexécution)
- [Components ECS](#components-ecs)
- [Managers (API Pull)](#managers-api-pull)
- [Systèmes ECS](#systèmes-ecs)
- [Exemples rapides](#exemples-rapides)
- [Liens utiles](#liens-utiles)

---

## Vue d'ensemble

### Qu'est-ce que le système Input ECS ?

Le système Input ECS d'Olympe Engine est une architecture moderne qui gère les entrées utilisateur (clavier, souris, manettes) en s'intégrant parfaitement avec l'Entity Component System (ECS) du moteur.

**Caractéristiques principales :**
- ✅ Support multi-joueurs local (plusieurs manettes + clavier)
- ✅ Hot-plug automatique (connexion/déconnexion à chaud)
- ✅ Mapping flexible hardware → gameplay
- ✅ Architecture découplée (hardware / gameplay)
- ✅ Compatible SDL3
- ✅ Thread-safe avec gestion d'événements

### Pourquoi cette architecture ?

#### Avantages vs ancienne approche :

**Avant (approche monolithique) :**
```cpp
// Code couplé, difficile à maintenir
if (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
        player.Move(); // ❌ Couplage fort
    }
}
```

**Maintenant (architecture ECS) :**
```cpp
// Découplage, flexible, testable
// 1. Hardware détecté par les Managers
// 2. State stocké dans les Components
// 3. Logique gameplay dans les Systems
// ✅ Séparation des responsabilités
```

**Bénéfices :**
- 🔧 **Maintenabilité** : chaque couche a une responsabilité claire
- 🎮 **Multi-joueur** : support natif de plusieurs contrôleurs
- 🔄 **Flexibilité** : ajout facile de nouveaux périphériques
- 🧪 **Testabilité** : possibilité de tester chaque couche isolément
- ⚡ **Performance** : cache d'entités, systèmes optimisés

---

## Concepts clés

Le système est organisé en **5 couches** distinctes :

```
┌─────────────────────────────────────────────────────────┐
│                    HARDWARE LAYER                        │
│           SDL3 Events, Physical Devices                  │
│    (Keyboard, Mouse, Joystick/Gamepad)                  │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                     STATE LAYER                          │
│         KeyboardManager, JoystickManager,                │
│              MouseManager, InputsManager                 │
│         (Capture events → Store state)                   │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                    MAPPING LAYER                         │
│               PlayerControlSystem                        │
│      (Maps hardware input → gameplay actions)            │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                     ECS LAYER                            │
│         Components: PlayerBinding, Controller,           │
│          PlayerController, Position, etc.                │
│              (Data storage in ECS)                       │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                   GAMEPLAY LAYER                         │
│         MovementSystem, AISystem, etc.                   │
│         (Use PlayerController data)                      │
└─────────────────────────────────────────────────────────┘
```

### Hardware Layer

**Périphériques physiques** gérés par SDL3 :
- 🎮 **Gamepad/Joystick** : manettes de jeu (Xbox, PlayStation, etc.)
- ⌨️ **Keyboard** : clavier (scancodes SDL)
- 🖱️ **Mouse** : souris (position, boutons, molette)

Les événements SDL3 (`SDL_Event`) sont capturés dans la boucle principale.

### State Layer

**Managers** qui transforment les événements SDL en état accessible :

| Manager | Rôle | API Pull |
|---------|------|----------|
| `KeyboardManager` | Gère l'état des touches | `IsKeyHeld()`, `IsKeyPressed()`, `IsKeyReleased()` |
| `JoystickManager` | Gère les manettes connectées | `GetAxis()`, `GetButton()`, `IsButtonPressed()` |
| `MouseManager` | Gère la souris | Position, delta, boutons |
| `InputsManager` | Orchestre les bindings joueurs ↔ devices | `BindControllerToPlayer()`, `AutoBindControllerToPlayer()` |

### Mapping Layer

**PlayerControlSystem** : système ECS qui lit l'état hardware et écrit dans les components gameplay.

- Lit : `Controller_data` (état brut du device)
- Écrit : `PlayerController_data` (actions gameplay)

### ECS Layer

**Components** qui stockent les données d'input :

| Component | Description |
|-----------|-------------|
| `PlayerBinding_data` | Lie un joueur à un device (keyboard = -1, joystick = ID) |
| `Controller_data` | État hardware brut du contrôleur (boutons, ID) |
| `PlayerController_data` | État gameplay abstrait (jump, shoot, direction) |
| `Position_data` | Position de l'entité (utilisé pour le mouvement) |

### Gameplay Layer

**Systèmes** qui utilisent les inputs pour la logique de jeu :
- `MovementSystem` : déplace les entités selon `PlayerController_data`
- `AISystem` : comportements IA
- `CollisionSystem` : détection de collisions
- etc.

---

## Architecture

### Schéma d'architecture visuel

```
           ┌──────────────┐
           │  SDL3 Events │
           └──────┬───────┘
                  │
        ┌─────────▼─────────┐
        │ InputsManager     │ ◄────┐
        │  HandleEvent()    │      │
        └─┬────────┬────────┘      │
          │        │                │
    ┌─────▼──┐  ┌─▼────────┐  ┌───▼────┐
    │Keyboard│  │ Joystick │  │ Mouse  │
    │Manager │  │ Manager  │  │Manager │
    └────┬───┘  └────┬─────┘  └────┬───┘
         │           │             │
         │     Post Events         │
         └──────┬────┴─────────────┘
                ▼
        ┌───────────────┐
        │ EventManager  │
        └───────┬───────┘
                │
                ▼
     ┌──────────────────────┐
     │ PlayerControlSystem  │
     │   (ECS System)       │
     └──────────┬───────────┘
                │
        ┌───────▼───────┐
        │  ECS World    │
        │  Components:  │
        │  - Binding    │
        │  - Controller │
        │  - PlayerCtrl │
        └───────┬───────┘
                │
         Other Systems
      (Movement, AI, etc.)
```

### Flux des données

**Frame par frame :**

1. **Hardware** → SDL3 génère des événements (`SDL_EVENT_KEY_DOWN`, etc.)
2. **Capture** → `InputsManager::HandleEvent()` distribue aux managers
3. **State** → Managers stockent l'état et postent des messages
4. **Event** → `EventManager` distribue aux listeners
5. **Mapping** → `PlayerControlSystem::Process()` lit hardware et écrit gameplay
6. **Logic** → Autres systèmes lisent `PlayerController_data` pour agir

---

## Pipeline d'exécution

### Ordre frame par frame (SDL3)

```
┌────────────────────────────────────────────────────────────┐
│                    FRAME N                                  │
└────────────────────────────────────────────────────────────┘

1. SDL_AppEvent(&event)
   └─> Capture hardware events (keyboard, joystick, mouse)
   └─> InputsManager::HandleEvent(&event)
       └─> KeyboardManager::HandleEvent()
       └─> JoystickManager::HandleEvent()
       └─> MouseManager::HandleEvent()
            └─> Set state flags (m_keysPressedThisFrame, etc.)
            └─> Post messages to EventManager

2. SDL_AppIterate()
   └─> GameEngine::Process() (calculate delta time)
   └─> EventManager::Process()
       └─> Dispatch messages to listeners
   
3. World::Process()
   └─> PlayerControlSystem::Process()
       └─> For each entity with PlayerBinding + Controller:
           - Read hardware state from Controller_data
           - Map to gameplay actions in PlayerController_data
   └─> CameraSystem::Process()
       └─> Read input state (IsKeyPressed, IsKeyHeld)
   └─> Other Systems (Movement, AI, etc.)
       └─> Read PlayerController_data
       └─> Apply game logic

4. Render()
   └─> Draw entities

5. BeginFrame()
   └─> Clear transient states (m_keysPressedThisFrame, m_keysReleasedThisFrame)
   └─> Prepare for next frame

6. EndFrame()
   └─> Swap buffers, return to SDL3 event loop
```

**⚠️ Important (SDL3):**  
`BeginFrame()` is called at the **END** of the frame (step 5), not at the beginning. This ensures that:
- Events captured in `SDL_AppEvent()` set the state flags
- Systems in `SDL_AppIterate()` can read those state flags
- State is cleared only after all systems have processed the input

This order is critical for `IsKeyPressed()` to work correctly with SDL3's event loop.

### Diagramme de séquence

```
Player Presses 'W'
       │
       ▼
   SDL3 Event (SDL_EVENT_KEY_DOWN, scancode=W)
       │
       ▼
   InputsManager::HandleEvent()
       │
       ▼
   KeyboardManager::HandleEvent()
       │
       └─> PostKeyEvent()
              │
              ▼
          EventManager::AddMessage()
              │
              ▼
          [Event Queue]
              │
              ▼
   PlayerControlSystem::Process()
       │
       └─> Reads binding (Player 0 → Keyboard)
       └─> Checks if 'W' is pressed
       └─> Writes to PlayerController_data.Joydirection.y = -1
              │
              ▼
   MovementSystem::Process()
       │
       └─> Reads PlayerController_data.Joydirection
       └─> Updates Position_data (moves entity up)
```

---

## Components ECS

### PlayerBinding_data

**Description :** Lie une entité (joueur) à un périphérique d'entrée.

```cpp
struct PlayerBinding_data
{
    short playerIndex = 0;     // Index du joueur (0, 1, 2, ...)
    short controllerID = -1;   // ID du device (-1 = keyboard, >=0 = joystick ID)
};
```

**Cas d'usage :**
- Assigner le joueur 1 au clavier : `controllerID = -1`
- Assigner le joueur 2 à la manette : `controllerID = <joystick_id>`

**Exemple :**
```cpp
PlayerBinding_data binding;
binding.playerIndex = 0;        // Player 0
binding.controllerID = -1;      // Keyboard
World::Get().AddComponent<PlayerBinding_data>(entity, binding);
```

---

### Controller_data

**Description :** État hardware brut du contrôleur (boutons, connexion).

```cpp
struct Controller_data
{
    short controllerID = -1;   // ID du contrôleur assigné
    bool isConnected = false;  // Le contrôleur est-il connecté ?
    bool isVibrating = false;  // Vibration active ?
    bool buttonStates[30];     // États des boutons (true = pressé)
};
```

**Cas d'usage :**
- Vérifier si un bouton spécifique est pressé
- Détecter une déconnexion de manette

**Exemple :**
```cpp
Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);
if (ctrl.isConnected && ctrl.buttonStates[0]) {
    // Button 0 pressed
}
```

---

### PlayerController_data

**Description :** État gameplay abstrait (actions de haut niveau).

```cpp
struct PlayerController_data
{
    Vector Joydirection;        // Direction du joystick (-1 à 1 en x/y)
    bool isJumping = false;     // Action: sauter
    bool isShooting = false;    // Action: tirer
    bool isRunning = false;     // Action: courir
    bool isInteracting = false; // Action: interagir
    bool isUsingItem = false;   // Action: utiliser objet
};
```

**Cas d'usage :**
- Lire la direction pour déplacer le joueur
- Vérifier si le joueur saute, tire, etc.

**Exemple :**
```cpp
PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
if (pc.isJumping) {
    ApplyJumpPhysics(entity);
}
Position_data& pos = World::Get().GetComponent<Position_data>(entity);
pos.position += pc.Joydirection * speed * deltaTime;
```

---

### Position_data

**Description :** Position 2D/3D de l'entité.

```cpp
struct Position_data
{
    Vector position; // 2D/3D position (x, y, z)
};
```

**Exemple :**
```cpp
Position_data pos;
pos.position = Vector{100.f, 200.f, 0.f};
World::Get().AddComponent<Position_data>(entity, pos);
```

---

## Managers (API Pull)

### KeyboardManager

Singleton qui gère l'état du clavier.

**API publique :**
```cpp
class KeyboardManager
{
public:
    static KeyboardManager& Get();
    
    void Initialize();
    void Shutdown();
    void HandleEvent(const SDL_Event* ev);
    
    // Note: Pour l'instant, pas d'API pull directe dans le header,
    // mais les événements sont postés au EventManager
};
```

**Utilisation typique :**
```cpp
// Les événements clavier sont gérés automatiquement
// et postés au EventManager pour les listeners
```

**Scancodes courants :**
- `SDL_SCANCODE_W`, `SDL_SCANCODE_A`, `SDL_SCANCODE_S`, `SDL_SCANCODE_D`
- `SDL_SCANCODE_SPACE`, `SDL_SCANCODE_RETURN`
- `SDL_SCANCODE_ESCAPE`

---

### JoystickManager

Singleton qui gère les manettes connectées.

**API publique :**
```cpp
class JoystickManager
{
public:
    static JoystickManager& Get();
    
    void Initialize();
    void Shutdown();
    void HandleEvent(const SDL_Event* ev);
    
    std::vector<SDL_JoystickID> GetConnectedJoysticks();
    bool IsJoystickConnected(SDL_JoystickID id);
};
```

**Exemple :**
```cpp
auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
for (auto jid : joysticks) {
    if (JoystickManager::Get().IsJoystickConnected(jid)) {
        // Joystick is connected
    }
}
```

**Gestion deadzone :**
Les axes analogiques ont une deadzone gérée au niveau du `PlayerControlSystem`.

---

### MouseManager

Singleton qui gère la souris.

**API publique :**
```cpp
class MouseManager
{
public:
    static MouseManager& Get();
    
    void Initialize();
    void Shutdown();
    void HandleEvent(const SDL_Event* ev);
};
```

**Utilisation :**
Les événements souris sont postés au `EventManager` pour traitement.

---

### InputsManager

Singleton principal qui orchestre les bindings joueurs ↔ périphériques.

**API publique :**
```cpp
class InputsManager
{
public:
    static InputsManager& Get();
    
    // Device management
    int GetConnectedJoysticksCount() const;
    int GetConnectedKeyboardsCount() const;
    int GetMaxDevices() const;
    int GetAvailableJoystickCount() const;
    
    // Player binding
    bool BindControllerToPlayer(short playerID, SDL_JoystickID controller);
    bool UnbindControllerFromPlayer(short playerID);
    bool AutoBindControllerToPlayer(short playerID);
    
    // Query
    bool IsPlayerBound(short playerID) const;
    SDL_JoystickID GetPlayerBinding(short playerID) const;
    short GetPlayerForController(SDL_JoystickID controller) const;
    
    // Disconnection handling
    bool AddDisconnectedPlayer(short playerID, SDL_JoystickID old_controller);
    bool RemoveDisconnectedPlayer(short playerID);
    bool IsPlayerDisconnected(short playerID) const;
    
    // Player registry
    bool AddPlayerIndex(short playerID, Player* playerPtr);
    
    // Status
    string GetDevicesStatusUpdate();
};
```

**Exemples :**

**Bind keyboard to player 0 :**
```cpp
InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
```

**Auto-bind first available device :**
```cpp
if (InputsManager::Get().AutoBindControllerToPlayer(0)) {
    // Successfully bound
}
```

**Check connection status :**
```cpp
int numJoysticks = InputsManager::Get().GetConnectedJoysticksCount();
int available = InputsManager::Get().GetAvailableJoystickCount();
```

---

## Systèmes ECS

### PlayerControlSystem

**Rôle :** Mapper l'état hardware (Controller_data) vers l'état gameplay (PlayerController_data).

**Signature requise :**
- `Position_data`
- `PlayerController_data`
- `PlayerBinding_data`
- `Controller_data`

**Fonctionnement :**
```cpp
void PlayerControlSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
        Controller_data& ctrlData = World::Get().GetComponent<Controller_data>(entity);
        PlayerController_data& controller = World::Get().GetComponent<PlayerController_data>(entity);
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        
        // Check if the controller is bound correctly
        if (binding.controllerID != ctrlData.controllerID)
            continue; // Skip if not bound
        
        // TODO: Map hardware state to gameplay actions
        // Example: Read axis from JoystickManager
        // controller.Joydirection = GetAxisDirection(binding.controllerID);
        
        // Apply movement (example)
        pos.position += controller.Joydirection * 100.f * deltaTime;
    }
}
```

**Quand s'exécute-t-il ?**
- Après le traitement des événements (`EventManager::Process()`)
- Avant les autres systèmes gameplay (Movement, AI, etc.)

**Comment il lit les states :**
- Actuellement : lit depuis `Controller_data` (modifié par les événements)
- Futur possible : lecture directe depuis les Managers pour un état frame-parfait

---

## Exemples rapides

### Créer une entité contrôlable

```cpp
// 1. Create entity
EntityID player = World::Get().CreateEntity();

// 2. Add Position
Position_data pos;
pos.position = Vector{100.f, 100.f, 0.f};
World::Get().AddComponent<Position_data>(player, pos);

// 3. Add PlayerBinding
PlayerBinding_data binding;
binding.playerIndex = 0;
binding.controllerID = -1; // Keyboard
World::Get().AddComponent<PlayerBinding_data>(player, binding);

// 4. Add Controller state
Controller_data ctrl;
ctrl.controllerID = -1;
ctrl.isConnected = true;
World::Get().AddComponent<Controller_data>(player, ctrl);

// 5. Add PlayerController
World::Get().AddComponent<PlayerController_data>(player);

// 6. Bind controller to player
InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
```

### Lire les inputs dans un système

```cpp
class MyGameplaySystem : public ECS_System
{
public:
    MyGameplaySystem() {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    }
    
    void Process() override {
        for (EntityID entity : m_entities) {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            
            // Move entity based on direction
            pos.position += pc.Joydirection * 150.f * deltaTime;
            
            // Handle jump
            if (pc.isJumping) {
                // Apply jump logic
            }
        }
    }
};
```

---

## Liens utiles

- 📚 [Quick Start Guide](INPUT_QUICK_START.md) - Commencer en 5 minutes
- 🔧 [API Reference](INPUT_API_REFERENCE.md) - Documentation complète de l'API
- 💡 [Examples](INPUT_EXAMPLES.md) - Exemples pratiques et cas d'usage
- 🏗️ [Architecture](INPUT_ARCHITECTURE.md) - Architecture détaillée et décisions de design
- 🔄 [Migration Guide](INPUT_MIGRATION.md) - Migrer du code existant
- 🐛 [Troubleshooting](INPUT_TROUBLESHOOTING.md) - Résolution de problèmes courants

---

## Contributeurs

- **Nicolas Chereau** - Architecte principal
- Olympe Engine Team - 2025

---

**Dernière mise à jour :** 2025  
**Licence :** Voir LICENSE dans le repository
