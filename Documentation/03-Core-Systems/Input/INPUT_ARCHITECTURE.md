# 🏗️ Input System - Architecture Détaillée

> **Documentation technique de l'architecture du système Input ECS.**

---

## Table des matières

- [Diagrammes](#diagrammes)
- [Décisions de design](#décisions-de-design)
- [Performance](#performance)
- [Extension du système](#extension-du-système)
- [Comparaison avec d'autres moteurs](#comparaison-avec-dautres-moteurs)

---

## Diagrammes

### Architecture globale

```
┌─────────────────────────────────────────────────────────────────┐
│                        OLYMPE ENGINE                             │
│                         Input System                             │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  HARDWARE LAYER (SDL3)                                           │
│  ┌──────────┐  ┌───────────┐  ┌──────────┐                     │
│  │ Keyboard │  │ Joystick  │  │  Mouse   │                     │
│  └────┬─────┘  └─────┬─────┘  └────┬─────┘                     │
│       │              │              │                            │
│       └──────────────┴──────────────┘                            │
│                      │                                           │
│              SDL_Event Queue                                     │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  STATE LAYER (Managers)                                          │
│                                                                   │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐ │
│  │KeyboardManager   │  │JoystickManager   │  │MouseManager   │ │
│  │                  │  │                  │  │               │ │
│  │- HandleEvent()   │  │- HandleEvent()   │  │- HandleEvent()│ │
│  │- PostKeyEvent()  │  │- PostAxisEvent() │  │- PostButton() │ │
│  │                  │  │- OpenJoystick()  │  │- PostMotion() │ │
│  └────────┬─────────┘  └────────┬─────────┘  └───────┬───────┘ │
│           │                     │                     │          │
│           └─────────────────────┴─────────────────────┘          │
│                                 │                                │
│                    ┌────────────▼────────────┐                  │
│                    │   InputsManager         │                  │
│                    │                         │                  │
│                    │- BindControllerToPlayer │                  │
│                    │- m_playerBindings       │                  │
│                    │- m_playerDisconnected   │                  │
│                    └────────────┬────────────┘                  │
└─────────────────────────────────┼──────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│  EVENT LAYER (EventManager)                                      │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                     EventManager                          │  │
│  │                                                           │  │
│  │  - Message Queue                                          │  │
│  │  - Register(listener, eventType)                          │  │
│  │  - AddMessage(msg)                                        │  │
│  │  - Process() → Dispatch messages to listeners            │  │
│  └──────────────────────┬───────────────────────────────────┘  │
└─────────────────────────┼──────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  MAPPING LAYER (ECS Systems)                                     │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │          PlayerControlSystem : ECS_System                 │  │
│  │                                                           │  │
│  │  Required Signature:                                      │  │
│  │    - Position_data                                        │  │
│  │    - PlayerBinding_data                                   │  │
│  │    - Controller_data                                      │  │
│  │    - PlayerController_data                                │  │
│  │                                                           │  │
│  │  Process():                                               │  │
│  │    1. Check binding matches (controllerID)                │  │
│  │    2. Read hardware state (future: from managers)         │  │
│  │    3. Map to gameplay (Joydirection, actions)             │  │
│  │    4. Apply movement                                      │  │
│  └──────────────────────┬───────────────────────────────────┘  │
└─────────────────────────┼──────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ECS LAYER (Components & World)                                  │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                     ECS World                             │  │
│  │                                                           │  │
│  │  ComponentPools:                                          │  │
│  │    - Pool<PlayerBinding_data>                             │  │
│  │    - Pool<Controller_data>                                │  │
│  │    - Pool<PlayerController_data>                          │  │
│  │    - Pool<Position_data>                                  │  │
│  │    - ... (other components)                               │  │
│  │                                                           │  │
│  │  Entities:                                                │  │
│  │    - EntityID → ComponentSignature                        │  │
│  └──────────────────────┬───────────────────────────────────┘  │
└─────────────────────────┼──────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  GAMEPLAY LAYER (Game Systems)                                   │
│                                                                   │
│  ┌───────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │MovementSystem │  │  AISystem    │  │CollisionSys  │         │
│  │               │  │              │  │              │         │
│  │Read:          │  │Read:         │  │Read:         │         │
│  │ -PlayerCtrl   │  │ -Position    │  │ -BoundingBox │         │
│  │ -Position     │  │ -AIBehavior  │  │ -Position    │         │
│  │               │  │              │  │              │         │
│  │Write:         │  │Write:        │  │Write:        │         │
│  │ -Position     │  │ -Movement    │  │ -Position    │         │
│  └───────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
```

### Flux des données détaillé

```
FRAME N

Time: 0ms ───────────────────────────────────────────────────────
             │
             ▼
   ┌─────────────────────┐
   │  BeginFrame()       │
   │  - Clear transient  │
   │    states           │
   └──────────┬──────────┘
              │
Time: 1ms ────▼──────────────────────────────────────────────────
   ┌─────────────────────────────────────────┐
   │  SDL_PollEvent(&event) LOOP             │
   │                                          │
   │  while (SDL_PollEvent(&event)) {         │
   │    GameEngine::HandleEvent(&event);      │
   │  }                                       │
   └──────────────────┬──────────────────────┘
                      │
                      ▼
   ┌──────────────────────────────────────────┐
   │  InputsManager::HandleEvent(&event)      │
   │                                          │
   │  Distributes to:                         │
   │   - KeyboardManager::HandleEvent()       │
   │   - JoystickManager::HandleEvent()       │
   │   - MouseManager::HandleEvent()          │
   └──────────────────┬──────────────────────┘
                      │
Time: 2ms ────────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  Manager::PostXXXEvent()                 │
   │                                          │
   │  Creates Message and posts to:           │
   │   EventManager::AddMessage(msg)          │
   │                                          │
   │  Message types:                          │
   │   - Keyboard_KeyDown                     │
   │   - Joystick_ButtonDown                  │
   │   - Joystick_AxisMotion                  │
   │   - etc.                                 │
   └──────────────────┬──────────────────────┘
                      │
Time: 3ms ────────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  EventManager::Process()                 │
   │                                          │
   │  for each Message in queue:              │
   │    - Find registered listeners           │
   │    - Call listener->OnEvent(msg)         │
   │                                          │
   │  Listeners can be:                       │
   │   - Game systems                         │
   │   - UI components                        │
   │   - Custom handlers                      │
   └──────────────────┬──────────────────────┘
                      │
Time: 4ms ────────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  World::ProcessSystems()                 │
   │                                          │
   │  for each System in order:               │
   │    system->Process()                     │
   │                                          │
   │  System execution order:                 │
   │   1. PlayerControlSystem                 │
   │   2. AISystem                            │
   │   3. PhysicsSystem                       │
   │   4. MovementSystem                      │
   │   5. CollisionSystem                     │
   │   6. ... (other systems)                 │
   └──────────────────┬──────────────────────┘
                      │
Time: 5ms ────────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  PlayerControlSystem::Process()          │
   │                                          │
   │  for each entity with required signature:│
   │    - Get PlayerBinding_data              │
   │    - Get Controller_data                 │
   │    - Get PlayerController_data           │
   │    - Get Position_data                   │
   │                                          │
   │    if (binding.controllerID ==           │
   │        ctrl.controllerID) {              │
   │      // Map hardware → gameplay          │
   │      pos += controller.Joydirection;     │
   │    }                                     │
   └──────────────────┬──────────────────────┘
                      │
Time: 6-10ms ─────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  Other Systems Process()                 │
   │                                          │
   │  - MovementSystem: move entities         │
   │  - AISystem: update AI behavior          │
   │  - PhysicsSystem: apply forces           │
   │  - CollisionSystem: detect collisions    │
   │  - TriggerSystem: check triggers         │
   └──────────────────┬──────────────────────┘
                      │
Time: 11-15ms ────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  Render()                                │
   │                                          │
   │  World::RenderSystems()                  │
   │    - RenderingSystem::Render()           │
   │      - Iterate entities with sprites     │
   │      - Draw at Position_data location    │
   └──────────────────┬──────────────────────┘
                      │
Time: 16ms ───────────▼──────────────────────────────────────────
   ┌──────────────────────────────────────────┐
   │  EndFrame()                              │
   │  - SDL_GL_SwapWindow()                   │
   │  - Wait for vsync (60 FPS)               │
   └──────────────────┬──────────────────────┘
                      │
                      ▼
                  FRAME N+1
```

### Relations entre components

```
┌──────────────────────────────────────────────────────────────┐
│                        Entity                                 │
│                    (EntityID = 42)                            │
└──────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│PlayerBinding │   │Controller    │   │PlayerCtrl    │
│              │   │              │   │              │
│playerIndex:0 │   │controllerID: │   │Joydirection: │
│controllerID: │◄──┤     -1       │   │  {0.5, 0}    │
│     -1       │   │isConnected:  │   │isJumping:    │
│              │   │     true     │   │  false       │
└──────────────┘   └──────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                            ▼
                   ┌──────────────┐
                   │Position      │
                   │              │
                   │position:     │
                   │ {100, 200, 0}│
                   └──────────────┘

RELATIONS:

1. PlayerBinding.controllerID MUST MATCH Controller.controllerID
   - This is checked in PlayerControlSystem::Process()
   - If mismatch, entity is skipped

2. PlayerController.Joydirection is READ by MovementSystem
   - MovementSystem updates Position based on Joydirection

3. Controller.isConnected is monitored for hotplug
   - DisconnectedPlayerSystem checks this flag
   - Auto-reconnect logic uses InputsManager bindings
```

---

## Décisions de design

### Pourquoi séparer hardware/gameplay ?

**Problème :**
Sans séparation, le code gameplay est couplé aux détails hardware spécifiques.

**Avant (couplé) :**
```cpp
// ❌ Mauvais : couplage fort
void PlayerUpdate()
{
    if (SDL_GetKeyboardState()[SDL_SCANCODE_W]) {
        player.y -= 5; // Hardware directement dans gameplay
    }
}
```

**Après (découplé) :**
```cpp
// ✅ Bon : séparation des responsabilités

// Layer 1: Hardware → State
void KeyboardManager::HandleEvent(SDL_Event* ev)
{
    PostKeyEvent(ev->key); // State management
}

// Layer 2: State → Gameplay abstraction
void PlayerControlSystem::Process()
{
    // Map hardware to gameplay actions
    playerCtrl.Joydirection = GetDirectionFromInput();
}

// Layer 3: Gameplay logic
void MovementSystem::Process()
{
    // Pure gameplay logic
    position += playerCtrl.Joydirection * speed * dt;
}
```

**Avantages :**
- ✅ **Testabilité** : on peut tester le gameplay sans hardware
- ✅ **Flexibilité** : changement de device sans modifier le gameplay
- ✅ **Portabilité** : support de nouveaux périphériques facilement
- ✅ **Replay/AI** : on peut "simuler" des inputs sans hardware

---

### Pourquoi un cache d'entités dans InputsManager ?

**Problème :**
Sans cache, on doit parcourir toutes les entités du World pour trouver les joueurs.

**Solution : Cache (m_playerIndex) :**
```cpp
std::unordered_map<short, Player*> m_playerIndex;
```

**Avantages :**
- ⚡ **Performance** : accès O(1) au lieu de O(n)
- 🎯 **Précision** : on sait exactement quels joueurs existent
- 🔄 **Hotplug** : on peut rapidement rebinder un joueur

**Utilisation :**
```cpp
// Enregistrer un joueur
InputsManager::Get().AddPlayerIndex(0, playerPtr);

// Accès rapide
Player* player = m_playerIndex[0];
player->m_ControllerID = joystickID;
```

---

### Pourquoi des contextes ? (Phase 2 - Non implémenté)

**Problème :**
Quand on ouvre un menu, on ne veut pas que le joueur continue à se déplacer dans le jeu.

**Solution prévue : Contextes (Push/Pop stack) :**
```cpp
enum class InputContext
{
    Gameplay,  // Contrôles de jeu normaux
    UI,        // Navigation dans les menus
    Editor     // Outils d'édition
};

// Stack de contextes
std::stack<InputContext> m_contextStack;

void PushContext(InputContext ctx); // Ajouter un contexte
void PopContext();                  // Retirer le contexte actuel
```

**Comportement :**
```
Initial state:  [Gameplay]
Open menu:      [Gameplay, UI]        ← UI inputs only
Close menu:     [Gameplay]            ← Back to gameplay
Open editor:    [Gameplay, UI, Editor]← Editor inputs only
```

**Avantages :**
- 🎯 **Filtrage automatique** : seuls les inputs du contexte actif passent
- 📚 **Historique** : on peut revenir au contexte précédent facilement
- 🔒 **Isolation** : UI et gameplay ne se perturbent pas

---

### Pull vs Push : quand utiliser quoi ?

**Deux paradigmes d'input :**

#### 1. Push (Event-driven)

**Principe :** Les événements sont "poussés" vers les listeners.

```cpp
class MyListener : public Object
{
    void OnEvent(const Message& msg) override
    {
        if (msg.msg_type == Olympe_EventType_Keyboard_KeyDown)
        {
            // Réagir immédiatement à l'événement
        }
    }
};
```

**Utilisations idéales :**
- ✅ Actions ponctuelles (appui unique, pas répété)
  - Ouvrir un menu (ESC)
  - Pause du jeu (START)
  - Quick save (F5)
- ✅ UI (clic de bouton, saisie de texte)
- ✅ Hotplug (connexion/déconnexion de device)

**Avantages :**
- Réactivité immédiate
- Pas de polling constant
- Bon pour les événements rares

---

#### 2. Pull (Polling)

**Principe :** On "tire" l'état actuel quand on en a besoin.

```cpp
void Process()
{
    const bool* keys = SDL_GetKeyboardState();
    if (keys[SDL_SCANCODE_W])
    {
        // Mouvement continu tant que W est appuyé
    }
}
```

**Utilisations idéales :**
- ✅ Mouvement continu (déplacement du joueur)
- ✅ États maintenus (tir continu, sprint)
- ✅ Axes analogiques (stick de manette)

**Avantages :**
- Cohérence frame-parfaite
- Plus simple pour les actions continues
- Pas de latence due à la queue d'événements

---

**Dans Olympe Engine :**

```
┌─────────────────────────────────────────────────────────────┐
│  Actuellement (Phase 1)                                      │
│                                                              │
│  Push (Event):                                               │
│    - Hotplug (connexion/déconnexion)                        │
│    - Messages postés au EventManager                        │
│                                                              │
│  Pull (Polling):                                             │
│    - Lecture directe via SDL_GetKeyboardState()             │
│    - Lecture directe via SDL_GetJoystickAxis()              │
│    - Dans les systèmes ECS                                  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Futur (Phase 2)                                             │
│                                                              │
│  Hybrid approach:                                            │
│    - Events pour les actions ponctuelles                    │
│    - State snapshot dans les managers (BeginFrame)          │
│    - Pull depuis les managers (frame-perfect)               │
└─────────────────────────────────────────────────────────────┘
```

---

## Performance

### Optimisations implémentées

#### 1. **Cache d'entités dans les systèmes**

```cpp
class ECS_System
{
    std::set<EntityID> m_entities; // Cache des entités pertinentes
};
```

**Avantage :** O(n) où n = nombre d'entités pertinentes, pas toutes les entités du World.

**Exemple :**
- 10 000 entités dans le World
- 4 joueurs avec PlayerController
- PlayerControlSystem itère sur 4 entités, pas 10 000

---

#### 2. **Component pools contigus**

Les components sont stockés dans des pools contigus en mémoire.

```cpp
template<typename T>
class ComponentPool
{
    std::vector<T> components; // Contiguïté = cache-friendly
};
```

**Avantage :** Cache CPU friendly, accès rapide.

---

#### 3. **Lookup O(1) pour les bindings**

```cpp
std::unordered_map<short, SDL_JoystickID> m_playerBindings;
```

**Avantage :** Trouver le binding d'un joueur est instantané.

---

#### 4. **Thread-safety avec mutex**

```cpp
class KeyboardManager
{
    std::mutex m_mutex; // Protection pour les accès concurrents
};
```

**Avantage :** Pas de race conditions si les événements viennent de différents threads.

---

### Overhead du système

**Mesures estimées (non benchmarkées) :**

| Opération | Coût | Notes |
|-----------|------|-------|
| `HandleEvent()` | ~0.01ms | Par événement SDL |
| `PostMessage()` | ~0.005ms | Ajout à la queue |
| `EventManager::Process()` | ~0.1-0.5ms | Pour tous les messages frame |
| `PlayerControlSystem::Process()` | ~0.01-0.05ms | Pour 4 joueurs |
| **Total overhead** | **~0.5-1ms** | Sur un frame 16ms (60 FPS) = 3-6% |

**Conclusion :** L'overhead est négligeable pour la plupart des jeux.

---

### Best practices

#### ✅ **DO:**

1. **Utiliser les systèmes ECS pour la logique gameplay**
   ```cpp
   // ✅ Bon
   class MovementSystem : public ECS_System
   {
       void Process() override { /* ... */ }
   };
   ```

2. **Lire l'état frame-parfait avec Pull**
   ```cpp
   // ✅ Bon pour mouvement continu
   const bool* keys = SDL_GetKeyboardState();
   if (keys[SDL_SCANCODE_W]) { /* move */ }
   ```

3. **Utiliser les événements pour les actions ponctuelles**
   ```cpp
   // ✅ Bon pour menu pause
   void OnEvent(const Message& msg)
   {
       if (msg.msg_type == Keyboard_KeyDown && 
           msg.controlId == SDL_SCANCODE_ESCAPE)
       {
           PauseGame();
       }
   }
   ```

---

#### ❌ **DON'T:**

1. **Ne pas accéder directement à SDL depuis le gameplay**
   ```cpp
   // ❌ Mauvais : couplage fort
   void GameLogic()
   {
       SDL_Event event;
       SDL_PollEvent(&event); // Non !
   }
   ```

2. **Ne pas itérer sur toutes les entités**
   ```cpp
   // ❌ Mauvais : O(n) inutile
   for (EntityID e = 0; e < 10000; e++)
   {
       if (World::Get().HasComponent<PlayerController_data>(e))
       {
           // ...
       }
   }
   
   // ✅ Bon : utiliser le système ECS
   class MySystem : public ECS_System
   {
       void Process() override
       {
           for (EntityID e : m_entities) // Seulement les pertinentes
           {
               // ...
           }
       }
   };
   ```

3. **Ne pas stocker des pointeurs d'entités long-terme**
   ```cpp
   // ❌ Mauvais : le component peut être déplacé
   PlayerController_data* pc = &World::Get().GetComponent<PlayerController_data>(entity);
   // ... temps passe ...
   pc->Joydirection = ...; // ⚠️ Peut pointer vers mémoire invalide
   
   // ✅ Bon : récupérer à chaque frame
   void Process()
   {
       PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
       pc.Joydirection = ...; // OK
   }
   ```

---

## Extension du système

### Ajouter un nouveau type de périphérique

**Exemple : Support du clavier/souris pour viser (FPS)**

#### 1. Créer un nouveau manager (si nécessaire)

```cpp
// Pas nécessaire : MouseManager existe déjà
```

#### 2. Ajouter un nouveau component

```cpp
// Dans ECS_Components.h
struct AimController_data
{
    Vector aimDirection;     // Direction de visée (normalisée)
    float sensitivity = 1.0f; // Sensibilité de la souris
};
```

#### 3. Créer un système de mapping

```cpp
// Dans ECS_Systems.h/.cpp
class AimControlSystem : public ECS_System
{
public:
    AimControlSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<AimController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    }
    
    void Process() override
    {
        // Read mouse position/delta
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        
        for (EntityID entity : m_entities)
        {
            AimController_data& aim = World::Get().GetComponent<AimController_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            
            // Calculate aim direction from player position to mouse
            Vector aimDir = Vector{mouseX, mouseY, 0.f} - pos.position;
            aim.aimDirection = aimDir.Normalized();
        }
    }
};
```

#### 4. Enregistrer le système

```cpp
auto aimSystem = std::make_shared<AimControlSystem>();
World::Get().RegisterSystem(aimSystem);
```

---

### Ajouter de nouvelles actions

**Exemple : Ajouter une action "Crouch" (accroupi)**

#### 1. Modifier PlayerController_data

```cpp
struct PlayerController_data
{
    Vector Joydirection;
    bool isJumping = false;
    bool isShooting = false;
    bool isRunning = false;
    bool isInteracting = false;
    bool isUsingItem = false;
    bool isCrouching = false; // ✅ Nouveau
};
```

#### 2. Mapper l'input dans le système

```cpp
void MyInputSystem::Process()
{
    const bool* keys = SDL_GetKeyboardState();
    
    // ...
    
    pc.isCrouching = keys[SDL_SCANCODE_LCTRL];
}
```

#### 3. Utiliser l'action dans un système gameplay

```cpp
class CrouchSystem : public ECS_System
{
public:
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            BoundingBox_data& bbox = World::Get().GetComponent<BoundingBox_data>(entity);
            
            if (pc.isCrouching)
            {
                bbox.boundingBox.h = 16.f; // Hitbox plus petite
            }
            else
            {
                bbox.boundingBox.h = 32.f; // Hitbox normale
            }
        }
    }
};
```

---

### Support VR/Touch (futur)

**Architecture prévue :**

```cpp
// Nouveau manager
class VRControllerManager : public Object
{
public:
    void Initialize();
    void HandleEvent(const SDL_Event* ev); // VR events
    
    Vector GetLeftControllerPosition();
    Vector GetRightControllerPosition();
    Quaternion GetLeftControllerRotation();
    Quaternion GetRightControllerRotation();
    
    bool IsLeftTriggerPressed();
    bool IsRightTriggerPressed();
};

// Nouveau component
struct VRController_data
{
    Vector leftHandPosition;
    Vector rightHandPosition;
    Quaternion leftHandRotation;
    Quaternion rightHandRotation;
    bool leftTrigger = false;
    bool rightTrigger = false;
};

// Nouveau système
class VRControlSystem : public ECS_System
{
public:
    void Process() override
    {
        // Map VR input to gameplay actions
    }
};
```

**Compatibilité :**
- ✅ L'architecture ECS permet d'ajouter VR sans modifier le code existant
- ✅ Les entités peuvent avoir à la fois VRController_data et PlayerController_data
- ✅ Les systèmes peuvent coexister

---

## Comparaison avec d'autres moteurs

### Unity Input System

| Caractéristique | Unity Input System | Olympe Input ECS |
|-----------------|-------------------|-------------------|
| **Architecture** | Asset-based + Action Maps | ECS + Managers |
| **Rebinding** | ✅ Built-in UI | ⏳ Phase 2 |
| **Multi-device** | ✅ Per-player | ✅ Per-player |
| **Hotplug** | ✅ Automatic | ✅ Automatic |
| **API** | High-level abstraction | Low-level + ECS |
| **Performance** | Good | Excellent (cache-friendly) |
| **Complexity** | Medium-High | Medium |

**Avantage Unity :** UI de rebinding prête à l'emploi, abstractions de haut niveau  
**Avantage Olympe :** Performance ECS, contrôle bas-niveau, personnalisable

---

### Unreal Enhanced Input

| Caractéristique | Unreal Enhanced Input | Olympe Input ECS |
|-----------------|----------------------|-------------------|
| **Architecture** | Blueprint + C++ | Pure C++ ECS |
| **Context system** | ✅ Built-in | ⏳ Phase 2 |
| **Modifiers** | ✅ (deadzone, scaling) | ✅ Manual |
| **Triggers** | ✅ (hold, tap, chord) | ⏳ Phase 2 |
| **Multi-device** | ✅ | ✅ |
| **Blueprint support** | ✅ | ⏳ Phase 2 |

**Avantage Unreal :** Modifiers/Triggers avancés, Blueprint integration  
**Avantage Olympe :** Simplicité, pas de dépendances lourdes

---

### Godot Input

| Caractéristique | Godot Input | Olympe Input ECS |
|-----------------|-------------|-------------------|
| **Architecture** | Action-based (strings) | ECS components |
| **Configuration** | Project settings | Code-based |
| **Multi-device** | ✅ | ✅ |
| **Simplicity** | ✅ Very simple | Medium |
| **Performance** | Good | Excellent (ECS) |

**Avantage Godot :** Extrêmement simple, configuration visuelle  
**Avantage Olympe :** Performance ECS, flexibilité du code

---

## Roadmap

### Phase 1 (Actuelle) ✅
- ✅ Multi-joueur local
- ✅ Hotplug automatique
- ✅ Architecture ECS
- ✅ Managers thread-safe

### Phase 2 (Prochain) ⏳
- ⏳ Contextes (UI/Gameplay/Editor)
- ⏳ Rebinding dynamique via UI
- ⏳ Profils de contrôles sauvegardés
- ⏳ InputMapping_data utilisé

### Phase 3 (Futur) 🔮
- 🔮 Vibration/Feedback haptique
- 🔮 Support VR/AR
- 🔮 Support touch (mobile)
- 🔮 Modifiers (deadzone, scaling, inversion)
- 🔮 Triggers (hold, tap, double-tap, chord)
- 🔮 Input recording/replay

---

## Glossaire

| Terme | Définition |
|-------|------------|
| **ECS** | Entity Component System - architecture de données |
| **Component** | Données pures attachées à une entité |
| **System** | Logique qui opère sur des entités avec certains components |
| **Signature** | Combinaison de components requis pour un système |
| **Pull** | Polling - lire l'état actuel à la demande |
| **Push** | Event-driven - recevoir des notifications d'événements |
| **Hotplug** | Connexion/déconnexion à chaud d'un périphérique |
| **Deadzone** | Zone morte autour du centre d'un stick analogique |
| **Binding** | Association entre un joueur et un périphérique |
| **Context** | Mode d'input actif (gameplay, UI, editor, etc.) |
| **Scancode** | Code matériel d'une touche clavier (indépendant du layout) |

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
