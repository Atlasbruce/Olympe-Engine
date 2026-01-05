# 🔧 Input System - API Reference

> **Documentation complète de toutes les classes, méthodes et composants du système Input.**

---

## Table des matières

- [Managers](#managers)
  - [KeyboardManager](#keyboardmanager)
  - [JoystickManager](#joystickmanager)
  - [MouseManager](#mousemanager)
  - [InputsManager](#inputsmanager)
- [Components ECS](#components-ecs)
  - [PlayerBinding_data](#playerbinding_data)
  - [Controller_data](#controller_data)
  - [PlayerController_data](#playercontroller_data)
  - [Position_data](#position_data)
- [Systems](#systems)
  - [PlayerControlSystem](#playercontrolsystem)
  - [InputSystem](#inputsystem)
- [Enums & Types](#enums--types)
- [Événements](#événements)

---

## Managers

---

### KeyboardManager

**Header :** `Source/system/KeyboardManager.h`

Singleton qui gère les événements clavier et leur état.

#### Classe

```cpp
class KeyboardManager : public Object
{
public:
    KeyboardManager();
    virtual ~KeyboardManager();
    
    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }
    
    static KeyboardManager& GetInstance();
    static KeyboardManager& Get(); // Alias
    
    void Initialize();
    void Shutdown();
    void HandleEvent(const SDL_Event* ev);
    
private:
    std::mutex m_mutex;
    void PostKeyEvent(SDL_KeyboardEvent const& ke);
};
```

#### Méthodes publiques

##### `GetInstance()` / `Get()`

Retourne l'instance singleton du KeyboardManager.

```cpp
KeyboardManager& kb = KeyboardManager::Get();
```

**Paramètres :** Aucun  
**Retour :** `KeyboardManager&` - Référence au singleton  
**Thread-safe :** Oui

---

##### `Initialize()`

Initialise le KeyboardManager. Les événements clavier SDL sont automatiquement capturés.

```cpp
KeyboardManager::Get().Initialize();
```

**Paramètres :** Aucun  
**Retour :** `void`  
**Note :** Doit être appelé une fois au démarrage du moteur.

---

##### `Shutdown()`

Arrête le KeyboardManager et libère les ressources.

```cpp
KeyboardManager::Get().Shutdown();
```

**Paramètres :** Aucun  
**Retour :** `void`

---

##### `HandleEvent(const SDL_Event* ev)`

Traite un événement SDL et poste les messages clavier appropriés.

```cpp
SDL_Event event;
while (SDL_PollEvent(&event)) {
    KeyboardManager::Get().HandleEvent(&event);
}
```

**Paramètres :**
- `ev` : Pointeur vers un événement SDL (ne doit pas être `nullptr`)

**Retour :** `void`

**Événements traités :**
- `SDL_EVENT_KEY_DOWN` : Touche pressée
- `SDL_EVENT_KEY_UP` : Touche relâchée

**Messages postés :**
- `EventType::Olympe_EventType_Keyboard_KeyDown`
- `EventType::Olympe_EventType_Keyboard_KeyUp`

---

#### Scancodes SDL courants

| Scancode | Description |
|----------|-------------|
| `SDL_SCANCODE_W` | Touche W |
| `SDL_SCANCODE_A` | Touche A |
| `SDL_SCANCODE_S` | Touche S |
| `SDL_SCANCODE_D` | Touche D |
| `SDL_SCANCODE_SPACE` | Barre d'espace |
| `SDL_SCANCODE_RETURN` | Entrée |
| `SDL_SCANCODE_ESCAPE` | Échap |
| `SDL_SCANCODE_LSHIFT` | Shift gauche |
| `SDL_SCANCODE_LCTRL` | Ctrl gauche |
| `SDL_SCANCODE_UP` | Flèche haut |
| `SDL_SCANCODE_DOWN` | Flèche bas |
| `SDL_SCANCODE_LEFT` | Flèche gauche |
| `SDL_SCANCODE_RIGHT` | Flèche droite |

**Documentation complète :** [SDL3 Scancode Reference](https://wiki.libsdl.org/SDL3/SDL_Scancode)

---

### JoystickManager

**Header :** `Source/system/JoystickManager.h`

Singleton qui gère les manettes/joysticks connectés.

#### Classe

```cpp
class JoystickManager : public Object
{
public:
    JoystickManager();
    virtual ~JoystickManager();
    
    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }
    
    static JoystickManager& GetInstance();
    static JoystickManager& Get(); // Alias
    
    void Initialize();
    void Scan_Joysticks();
    void Shutdown();
    void Process(float dt);
    void HandleEvent(const SDL_Event* ev);
    
    std::vector<SDL_JoystickID> GetConnectedJoysticks();
    bool IsJoystickConnected(SDL_JoystickID id);
    
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
    
    std::unordered_map<SDL_JoystickID, JoystickInfo> m_joysticks;
    std::mutex m_mutex;
    
    void OpenJoystick(SDL_JoystickID instance_id);
    void CloseJoystick(SDL_JoystickID instance_id);
    void PostJoystickButtonEvent(SDL_JoystickID which, int button, bool down);
    void PostJoystickAxisEvent(SDL_JoystickID which, int axis, Sint16 value);
    void PostJoystickConnectedEvent(SDL_JoystickID which, bool bconnected);
};
```

#### Méthodes publiques

##### `Initialize()`

Initialise le JoystickManager et scanne les manettes déjà connectées.

```cpp
JoystickManager::Get().Initialize();
```

**Paramètres :** Aucun  
**Retour :** `void`  
**Note :** Active automatiquement les événements joystick SDL.

---

##### `Scan_Joysticks()`

Scanne et ouvre toutes les manettes actuellement connectées.

```cpp
JoystickManager::Get().Scan_Joysticks();
```

**Paramètres :** Aucun  
**Retour :** `void`

---

##### `GetConnectedJoysticks()`

Retourne la liste des IDs de toutes les manettes connectées.

```cpp
std::vector<SDL_JoystickID> joysticks = JoystickManager::Get().GetConnectedJoysticks();
for (auto jid : joysticks) {
    std::cout << "Joystick ID: " << jid << std::endl;
}
```

**Paramètres :** Aucun  
**Retour :** `std::vector<SDL_JoystickID>` - Liste des IDs de manettes

---

##### `IsJoystickConnected(SDL_JoystickID id)`

Vérifie si une manette spécifique est connectée.

```cpp
SDL_JoystickID jid = 1;
if (JoystickManager::Get().IsJoystickConnected(jid)) {
    std::cout << "Joystick " << jid << " is connected" << std::endl;
}
```

**Paramètres :**
- `id` : ID de la manette à vérifier

**Retour :** `bool` - `true` si connectée, `false` sinon

---

##### `HandleEvent(const SDL_Event* ev)`

Traite les événements SDL liés aux manettes.

```cpp
SDL_Event event;
while (SDL_PollEvent(&event)) {
    JoystickManager::Get().HandleEvent(&event);
}
```

**Événements traités :**
- `SDL_EVENT_JOYSTICK_ADDED` : Manette connectée
- `SDL_EVENT_JOYSTICK_REMOVED` : Manette déconnectée
- `SDL_EVENT_JOYSTICK_BUTTON_DOWN` : Bouton pressé
- `SDL_EVENT_JOYSTICK_BUTTON_UP` : Bouton relâché
- `SDL_EVENT_JOYSTICK_AXIS_MOTION` : Mouvement d'axe
- `SDL_EVENT_GAMEPAD_ADDED` : Gamepad ajouté
- `SDL_EVENT_GAMEPAD_REMOVED` : Gamepad retiré

---

### MouseManager

**Header :** `Source/system/MouseManager.h`

Singleton qui gère les événements souris.

#### Classe

```cpp
class MouseManager : public Object
{
public:
    MouseManager();
    virtual ~MouseManager();
    
    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }
    
    static MouseManager& GetInstance();
    static MouseManager& Get();
    
    void Initialize();
    void Shutdown();
    void HandleEvent(const SDL_Event* ev);
    
private:
    std::mutex m_mutex;
    void PostButtonEvent(const SDL_MouseButtonEvent& be);
    void PostMotionEvent(const SDL_MouseMotionEvent& me);
};
```

#### Méthodes publiques

##### `Initialize()`

Initialise le MouseManager.

```cpp
MouseManager::Get().Initialize();
```

---

##### `HandleEvent(const SDL_Event* ev)`

Traite les événements SDL liés à la souris.

```cpp
SDL_Event event;
while (SDL_PollEvent(&event)) {
    MouseManager::Get().HandleEvent(&event);
}
```

**Événements traités :**
- `SDL_EVENT_MOUSE_BUTTON_DOWN` : Bouton souris pressé
- `SDL_EVENT_MOUSE_BUTTON_UP` : Bouton souris relâché
- `SDL_EVENT_MOUSE_MOTION` : Mouvement de la souris
- `SDL_EVENT_MOUSE_WHEEL` : Molette de la souris

---

### InputsManager

**Header :** `Source/InputsManager.h`

Singleton principal qui orchestre les bindings entre joueurs et périphériques.

#### Classe

```cpp
class InputsManager : public Object
{
public:
    InputsManager();
    virtual ~InputsManager();
    
    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }
    
    static InputsManager& GetInstance();
    static InputsManager& Get();
    
    void Shutdown();
    virtual void HandleEvent(const SDL_Event* ev);
    virtual void OnEvent(const Message& msg) override;
    
    // Device queries
    bool IsKeyboardAssigned() const;
    string GetDevicesStatusUpdate();
    int GetConnectedJoysticksCount() const;
    int GetConnectedKeyboardsCount() const;
    int GetMaxDevices() const;
    int GetAvailableJoystickCount() const;
    
    // Player binding
    bool AutoBindControllerToPlayer(short playerID);
    bool AddPlayerIndex(short playerID, Player* playerPtr);
    bool BindControllerToPlayer(short playerID, SDL_JoystickID controller);
    bool UnbindControllerFromPlayer(short playerID);
    
    // Disconnection management
    bool AddDisconnectedPlayer(short playerID, SDL_JoystickID old_controller);
    bool RemoveDisconnectedPlayer(short playerID);
    bool IsPlayerDisconnected(short playerID) const;
    short GetDisconnectedPlayersCount() const;
    short GetFirstDisconnectedPlayerID() const;
    
    // Queries
    bool IsPlayerBound(short playerID) const;
    SDL_JoystickID GetPlayerBinding(short playerID) const;
    short GetPlayerForController(SDL_JoystickID controller) const;
    
private:
    std::unordered_map<short, SDL_JoystickID> m_playerBindings;
    std::unordered_map<short, SDL_JoystickID> m_playerDisconnected;
    std::unordered_map<short, Player*> m_playerIndex;
    bool m_keyboardAssigned = false;
};
```

#### Méthodes publiques

##### `GetConnectedJoysticksCount()`

Retourne le nombre de manettes connectées.

```cpp
int count = InputsManager::Get().GetConnectedJoysticksCount();
std::cout << count << " joystick(s) connected" << std::endl;
```

**Retour :** `int` - Nombre de manettes

---

##### `GetAvailableJoystickCount()`

Retourne le nombre de manettes non assignées à un joueur.

```cpp
int available = InputsManager::Get().GetAvailableJoystickCount();
std::cout << available << " unassigned joystick(s)" << std::endl;
```

**Retour :** `int` - Nombre de manettes disponibles

---

##### `BindControllerToPlayer(short playerID, SDL_JoystickID controller)`

Lie un contrôleur (manette ou clavier) à un joueur.

```cpp
// Bind keyboard to player 0
InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));

// Bind joystick ID 1 to player 1
InputsManager::Get().BindControllerToPlayer(1, SDL_JoystickID(1));
```

**Paramètres :**
- `playerID` : ID du joueur (0, 1, 2, ...)
- `controller` : ID du contrôleur
  - `-1` pour le clavier
  - `>= 0` pour un joystick

**Retour :** `bool`
- `true` : binding réussi
- `false` : échec (contrôleur déjà assigné ou non connecté)

**Notes :**
- Le clavier ne peut être assigné qu'à un seul joueur
- Une manette ne peut être assignée qu'à un seul joueur

---

##### `AutoBindControllerToPlayer(short playerID)`

Lie automatiquement le premier contrôleur disponible à un joueur.

```cpp
if (InputsManager::Get().AutoBindControllerToPlayer(0)) {
    std::cout << "Player 0 successfully bound" << std::endl;
} else {
    std::cerr << "No available controller for Player 0" << std::endl;
}
```

**Paramètres :**
- `playerID` : ID du joueur

**Retour :** `bool` - `true` si binding réussi

**Logique :**
1. Essaie de binder la première manette disponible
2. Si aucune manette, essaie de binder le clavier
3. Si échec, retourne `false`

---

##### `UnbindControllerFromPlayer(short playerID)`

Délie un contrôleur d'un joueur.

```cpp
InputsManager::Get().UnbindControllerFromPlayer(0);
```

**Paramètres :**
- `playerID` : ID du joueur

**Retour :** `bool` - `true` si unbinding réussi

---

##### `IsPlayerBound(short playerID)`

Vérifie si un joueur est lié à un contrôleur.

```cpp
if (InputsManager::Get().IsPlayerBound(0)) {
    std::cout << "Player 0 is bound to a controller" << std::endl;
}
```

**Paramètres :**
- `playerID` : ID du joueur

**Retour :** `bool` - `true` si le joueur est lié

---

##### `GetPlayerBinding(short playerID)`

Retourne l'ID du contrôleur lié à un joueur.

```cpp
SDL_JoystickID controller = InputsManager::Get().GetPlayerBinding(0);
if (controller == SDL_JoystickID(-1)) {
    std::cout << "Player 0 uses keyboard" << std::endl;
} else {
    std::cout << "Player 0 uses joystick " << controller << std::endl;
}
```

**Paramètres :**
- `playerID` : ID du joueur

**Retour :** `SDL_JoystickID`
- `-1` : clavier
- `>= 0` : ID de manette
- `0` : non lié

---

##### `GetPlayerForController(SDL_JoystickID controller)`

Retourne l'ID du joueur lié à un contrôleur donné.

```cpp
SDL_JoystickID jid = 1;
short playerID = InputsManager::Get().GetPlayerForController(jid);
if (playerID >= 0) {
    std::cout << "Joystick " << jid << " is bound to player " << playerID << std::endl;
} else {
    std::cout << "Joystick " << jid << " is not bound" << std::endl;
}
```

**Paramètres :**
- `controller` : ID du contrôleur

**Retour :** `short`
- `>= 0` : ID du joueur
- `-1` : non lié

---

##### `GetDevicesStatusUpdate()`

Retourne une chaîne de caractères décrivant l'état de tous les périphériques.

```cpp
std::string status = InputsManager::Get().GetDevicesStatusUpdate();
std::cout << status << std::endl;
```

**Retour :** `string` - Description formatée

**Exemple de sortie :**
```
---- InputsManager Devices Status ----
Connected Joysticks: 2
Available Unassigned Joysticks: 1
Available Assigned Joysticks: 1
  - Joystick ID=1  -> Bound to Player 0
  - Joystick ID=2  -> Not bound to any player
Keyboard: Assigned to Player 0
...
```

---

## Components ECS

---

### PlayerBinding_data

**Header :** `Source/ECS_Components.h`

Lie une entité (joueur) à un périphérique d'entrée.

#### Structure

```cpp
struct PlayerBinding_data
{
    short playerIndex = 0;    // Index du joueur (0, 1, 2, ...)
    short controllerID = -1;  // ID du contrôleur (-1 = keyboard, >= 0 = joystick)
};
```

#### Membres

| Membre | Type | Valeur par défaut | Description |
|--------|------|-------------------|-------------|
| `playerIndex` | `short` | `0` | Index du joueur (0 = Player 1, 1 = Player 2, etc.) |
| `controllerID` | `short` | `-1` | ID du contrôleur (-1 = clavier, >= 0 = ID joystick SDL) |

#### Utilisation

```cpp
// Bind player 0 to keyboard
PlayerBinding_data binding;
binding.playerIndex = 0;
binding.controllerID = -1; // Keyboard
World::Get().AddComponent<PlayerBinding_data>(entity, binding);

// Bind player 1 to joystick ID 1
PlayerBinding_data binding2;
binding2.playerIndex = 1;
binding2.controllerID = 1; // Joystick
World::Get().AddComponent<PlayerBinding_data>(entity2, binding2);
```

---

### Controller_data

**Header :** `Source/ECS_Components.h`

État hardware brut d'un contrôleur (boutons, connexion).

#### Structure

```cpp
struct Controller_data
{
    short controllerID = -1;   // ID du contrôleur
    bool isConnected = false;  // État de connexion
    bool isVibrating = false;  // Vibration active ?
    bool buttonStates[30];     // États des boutons (true = pressé)
};
```

#### Membres

| Membre | Type | Valeur par défaut | Description |
|--------|------|-------------------|-------------|
| `controllerID` | `short` | `-1` | ID du contrôleur (-1 = keyboard, >= 0 = joystick) |
| `isConnected` | `bool` | `false` | `true` si le contrôleur est connecté |
| `isVibrating` | `bool` | `false` | `true` si vibration active (futur) |
| `buttonStates[30]` | `bool[]` | `false` | État de chaque bouton (max 30 boutons) |

#### Utilisation

```cpp
Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);

// Check connection
if (!ctrl.isConnected) {
    std::cout << "Controller disconnected!" << std::endl;
}

// Check button state (example: button 0)
if (ctrl.buttonStates[0]) {
    std::cout << "Button 0 is pressed" << std::endl;
}
```

---

### PlayerController_data

**Header :** `Source/ECS_Components.h`

État gameplay abstrait (actions de haut niveau).

#### Structure

```cpp
struct PlayerController_data
{
    Vector Joydirection;        // Direction du joystick/clavier (normalisé -1 à 1)
    bool isJumping = false;     // Action: sauter
    bool isShooting = false;    // Action: tirer
    bool isRunning = false;     // Action: courir
    bool isInteracting = false; // Action: interagir
    bool isUsingItem = false;   // Action: utiliser un objet
};
```

#### Membres

| Membre | Type | Valeur par défaut | Description |
|--------|------|-------------------|-------------|
| `Joydirection` | `Vector` | `{0, 0, 0}` | Direction normalisée (-1 à 1) en x/y |
| `isJumping` | `bool` | `false` | `true` si le joueur saute |
| `isShooting` | `bool` | `false` | `true` si le joueur tire |
| `isRunning` | `bool` | `false` | `true` si le joueur court |
| `isInteracting` | `bool` | `false` | `true` si le joueur interagit |
| `isUsingItem` | `bool` | `false` | `true` si le joueur utilise un objet |

#### Utilisation

```cpp
PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);

// Read direction
Vector dir = pc.Joydirection;
position += dir * speed * deltaTime;

// Check actions
if (pc.isJumping) {
    ApplyJumpForce(entity);
}

if (pc.isShooting) {
    FireWeapon(entity);
}
```

---

### Position_data

**Header :** `Source/ECS_Components.h`

Position 2D/3D d'une entité.

#### Structure

```cpp
struct Position_data
{
    Vector position; // Position (x, y, z)
};
```

#### Membres

| Membre | Type | Description |
|--------|------|-------------|
| `position` | `Vector` | Coordonnées (x, y, z) |

#### Utilisation

```cpp
Position_data pos;
pos.position = Vector{100.f, 200.f, 0.f};
World::Get().AddComponent<Position_data>(entity, pos);

// Later: read/modify position
Position_data& pos = World::Get().GetComponent<Position_data>(entity);
pos.position.x += 10.f; // Move right
```

---

## Systems

---

### PlayerControlSystem

**Header :** `Source/ECS_Systems.h`  
**Implementation :** `Source/ECS_Systems.cpp`

Système ECS qui mappe l'état hardware vers l'état gameplay.

#### Classe

```cpp
class PlayerControlSystem : public ECS_System
{
public:
    PlayerControlSystem();
    virtual void Process() override;
};
```

#### Signature requise

Le système opère sur les entités ayant **tous** ces composants :
- `Position_data`
- `PlayerController_data`
- `PlayerBinding_data`
- `Controller_data`

#### Fonctionnement

```cpp
void PlayerControlSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        // Get components
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        PlayerController_data& controller = World::Get().GetComponent<PlayerController_data>(entity);
        PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
        Controller_data& ctrlData = World::Get().GetComponent<Controller_data>(entity);
        
        // Check binding matches
        if (binding.controllerID != ctrlData.controllerID)
            continue; // Skip if not bound correctly
        
        // Map hardware → gameplay (currently: direct movement)
        pos.position += controller.Joydirection * 100.f * deltaTime;
    }
}
```

**Note :** Actuellement, le mapping hardware → gameplay est basique. Dans une version future, ce système lira les événements ou l'état des managers pour remplir `PlayerController_data`.

---

### InputSystem

**Header :** `Source/ECS_Systems.h`

Système générique pour le traitement des inputs (actuellement vide).

#### Classe

```cpp
class InputSystem : public ECS_System
{
public:
    InputSystem();
    virtual void Process() override;
};
```

**Note :** Ce système est un placeholder pour une future logique d'input générique.

---

## Enums & Types

### SDL_JoystickID

Type SDL pour identifier un joystick.

```cpp
typedef Sint32 SDL_JoystickID;
```

**Valeurs spéciales :**
- `-1` : Représente le clavier dans le système Olympe
- `>= 0` : ID réel d'un joystick SDL

---

## Événements

### Messages postés par le système Input

Les managers d'input postent des messages au `EventManager` pour notifier les listeners.

| EventType | Émis par | Données |
|-----------|----------|---------|
| `Olympe_EventType_Keyboard_KeyDown` | KeyboardManager | `controlId` = scancode |
| `Olympe_EventType_Keyboard_KeyUp` | KeyboardManager | `controlId` = scancode |
| `Olympe_EventType_Joystick_Connected` | JoystickManager | `deviceId` = joystick ID |
| `Olympe_EventType_Joystick_Disconnected` | JoystickManager | `deviceId` = joystick ID |
| `Olympe_EventType_Joystick_ButtonDown` | JoystickManager | `deviceId`, `controlId` = button |
| `Olympe_EventType_Joystick_ButtonUp` | JoystickManager | `deviceId`, `controlId` = button |
| `Olympe_EventType_Joystick_AxisMotion` | JoystickManager | `deviceId`, `controlId` = axis, `param1` = value |

### S'abonner aux événements

```cpp
class MyInputListener : public Object
{
public:
    void OnEvent(const Message& msg) override
    {
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Keyboard_KeyDown:
                std::cout << "Key pressed: " << msg.controlId << std::endl;
                break;
                
            case EventType::Olympe_EventType_Joystick_Connected:
                std::cout << "Joystick connected: " << msg.deviceId << std::endl;
                break;
        }
    }
};

// Register listener
MyInputListener* listener = new MyInputListener();
EventManager::Get().Register(listener, EventType::Olympe_EventType_Keyboard_KeyDown);
EventManager::Get().Register(listener, EventType::Olympe_EventType_Joystick_Connected);
```

---

## Limites et Contraintes

### Limites actuelles

| Limite | Valeur | Notes |
|--------|--------|-------|
| Joueurs max | Illimité (limité par devices) | Pratiquement 1-8 joueurs |
| Boutons max par contrôleur | 30 | Défini dans `Controller_data.buttonStates[30]` |
| Clavier max | 1 | Un seul clavier peut être assigné |
| Manettes max | Selon SDL3 | Généralement 4-8 |

### Contraintes

- Le clavier ne peut être assigné qu'à **un seul joueur** à la fois
- Une manette ne peut être assignée qu'à **un seul joueur** à la fois
- Les IDs de manette sont gérés par SDL3 (peuvent changer à la déconnexion/reconnexion)
- Le mapping hardware → gameplay est actuellement basique (Phase 1)

---

## Notes de version

**Phase 1 (Actuelle) :**
- ✅ Support multi-joueur local
- ✅ Hotplug automatique
- ✅ Architecture ECS
- ✅ Managers thread-safe
- ⏳ Mapping hardware → gameplay basique

**Phase 2 (Futur) :**
- ⏳ Contextes (UI/Gameplay/Editor)
- ⏳ Rebinding dynamique
- ⏳ Profils de contrôles
- ⏳ Vibration/Feedback haptique
- ⏳ Support VR/Touch

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
