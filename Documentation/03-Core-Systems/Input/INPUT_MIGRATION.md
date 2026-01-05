# 🔄 Input System - Guide de Migration

> **Guide pour migrer du code existant vers le nouveau système Input ECS.**

---

## Table des matières

- [Vue d'ensemble](#vue-densemble)
- [Migration : Ancien système → ECS](#migration--ancien-système--ecs)
- [Exemples de migration](#exemples-de-migration)
- [Checklist de migration](#checklist-de-migration)
- [Problèmes courants](#problèmes-courants)

---

## Vue d'ensemble

### Pourquoi migrer ?

L'ancien système (direct SDL) présente plusieurs limitations :
- ❌ Couplage fort entre hardware et gameplay
- ❌ Difficile de supporter plusieurs joueurs
- ❌ Pas de gestion du hotplug
- ❌ Code difficile à tester

Le nouveau système ECS offre :
- ✅ Découplage hardware/gameplay
- ✅ Support multi-joueur natif
- ✅ Hotplug automatique
- ✅ Architecture testable

---

## Migration : Ancien système → ECS

### Pattern 1 : Input direct SDL

#### Avant (ancien système)

```cpp
// ❌ Ancien : Input direct dans la boucle de jeu
void GameLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.scancode == SDL_SCANCODE_W)
            {
                player.y -= 5; // Mouvement hardcodé
            }
        }
    }
}
```

#### Après (nouveau système ECS)

```cpp
// ✅ Nouveau : Système ECS découplé

// 1. Créer l'entité avec les components appropriés
EntityID player = World::Get().CreateEntity();

Position_data pos;
pos.position = Vector{100.f, 100.f, 0.f};
World::Get().AddComponent<Position_data>(player, pos);

PlayerBinding_data binding;
binding.playerIndex = 0;
binding.controllerID = -1; // Keyboard
World::Get().AddComponent<PlayerBinding_data>(player, binding);

World::Get().AddComponent<Controller_data>(player);
World::Get().AddComponent<PlayerController_data>(player);

InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));

// 2. Créer un système de mapping input
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
        const bool* keys = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            
            if (binding.controllerID != -1) continue; // Only keyboard
            
            pc.Joydirection = Vector{0.f, 0.f, 0.f};
            if (keys[SDL_SCANCODE_W]) pc.Joydirection.y = -1.f;
            if (keys[SDL_SCANCODE_S]) pc.Joydirection.y = 1.f;
            if (keys[SDL_SCANCODE_A]) pc.Joydirection.x = -1.f;
            if (keys[SDL_SCANCODE_D]) pc.Joydirection.x = 1.f;
        }
    }
};

// 3. Créer un système de mouvement
class MovementSystem : public ECS_System
{
public:
    MovementSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            pos.position += pc.Joydirection * 200.f * GameEngine::fDt;
        }
    }
};

// 4. Enregistrer les systèmes
World::Get().RegisterSystem(std::make_shared<KeyboardInputSystem>());
World::Get().RegisterSystem(std::make_shared<MovementSystem>());
```

---

### Pattern 2 : Classe Player monolithique

#### Avant (ancien système)

```cpp
// ❌ Ancien : Classe Player avec input direct
class Player
{
public:
    Vector position;
    Vector velocity;
    
    void Update(float dt)
    {
        // Input hardcodé dans la classe
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        
        velocity = Vector{0, 0, 0};
        if (keys[SDL_SCANCODE_W]) velocity.y = -1;
        if (keys[SDL_SCANCODE_S]) velocity.y = 1;
        if (keys[SDL_SCANCODE_A]) velocity.x = -1;
        if (keys[SDL_SCANCODE_D]) velocity.x = 1;
        
        position += velocity * 100.f * dt;
    }
    
    void Render()
    {
        // Rendering code
    }
};

// Usage
Player player;
player.Update(deltaTime);
player.Render();
```

#### Après (nouveau système ECS)

```cpp
// ✅ Nouveau : Séparation en components et systems

// 1. Components (données)
struct Position_data { Vector position; };
struct PlayerController_data { Vector Joydirection; };
struct PlayerBinding_data { short playerIndex; short controllerID; };

// 2. Système d'input (lecture hardware)
class PlayerInputSystem : public ECS_System
{
public:
    void Process() override
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            pc.Joydirection = Vector{0, 0, 0};
            if (keys[SDL_SCANCODE_W]) pc.Joydirection.y = -1;
            if (keys[SDL_SCANCODE_S]) pc.Joydirection.y = 1;
            if (keys[SDL_SCANCODE_A]) pc.Joydirection.x = -1;
            if (keys[SDL_SCANCODE_D]) pc.Joydirection.x = 1;
        }
    }
};

// 3. Système de mouvement (logique gameplay)
class MovementSystem : public ECS_System
{
public:
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            pos.position += pc.Joydirection * 100.f * GameEngine::fDt;
        }
    }
};

// 4. Système de rendu (séparé)
class RenderingSystem : public ECS_System
{
public:
    void Render() override
    {
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
            
            // Rendering code
        }
    }
};

// Usage
EntityID player = World::Get().CreateEntity();
World::Get().AddComponent<Position_data>(player);
World::Get().AddComponent<PlayerController_data>(player);
World::Get().AddComponent<PlayerBinding_data>(player);

World::Get().RegisterSystem(std::make_shared<PlayerInputSystem>());
World::Get().RegisterSystem(std::make_shared<MovementSystem>());
World::Get().RegisterSystem(std::make_shared<RenderingSystem>());
```

---

### Pattern 3 : Événements SDL directs

#### Avant (ancien système)

```cpp
// ❌ Ancien : Traitement d'événements dans le code gameplay
void HandleGameEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_SPACE)
                {
                    player.Jump(); // Appel direct
                }
                break;
                
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
                if (event.jbutton.button == 0) // A button
                {
                    player.Jump();
                }
                break;
        }
    }
}
```

#### Après (nouveau système ECS)

```cpp
// ✅ Nouveau : Utilisation du système d'événements

// 1. Système d'input qui map les événements
class JumpInputSystem : public ECS_System
{
public:
    void Process() override
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            
            // Keyboard
            if (binding.controllerID == -1)
            {
                pc.isJumping = keys[SDL_SCANCODE_SPACE];
            }
            // Gamepad
            else
            {
                SDL_Joystick* joy = SDL_GetJoystickFromID(
                    static_cast<SDL_JoystickID>(binding.controllerID));
                if (joy)
                {
                    pc.isJumping = SDL_GetJoystickButton(joy, 0); // A button
                }
            }
        }
    }
};

// 2. Système de gameplay qui utilise l'action
class JumpSystem : public ECS_System
{
public:
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            PhysicsBody_data& physics = World::Get().GetComponent<PhysicsBody_data>(entity);
            
            if (pc.isJumping && !wasJumping[entity])
            {
                // Apply jump force (edge detection: just pressed)
                physics.velocity.y = -500.f; // Jump velocity
            }
            
            wasJumping[entity] = pc.isJumping;
        }
    }
    
private:
    std::unordered_map<EntityID, bool> wasJumping;
};
```

---

## Exemples de migration

### Exemple complet : Jeu de plateforme simple

#### Avant (ancien système)

```cpp
// ❌ Ancien code monolithique

class PlatformerGame
{
    struct Player
    {
        float x, y;
        float vx, vy;
        bool onGround;
    };
    
    Player player;
    
    void Update(float dt)
    {
        // Input
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        
        player.vx = 0;
        if (keys[SDL_SCANCODE_A]) player.vx = -200;
        if (keys[SDL_SCANCODE_D]) player.vx = 200;
        if (keys[SDL_SCANCODE_SPACE] && player.onGround)
        {
            player.vy = -400; // Jump
            player.onGround = false;
        }
        
        // Physics
        player.vy += 980 * dt; // Gravity
        
        // Movement
        player.x += player.vx * dt;
        player.y += player.vy * dt;
        
        // Ground check (simplified)
        if (player.y >= 400)
        {
            player.y = 400;
            player.vy = 0;
            player.onGround = true;
        }
    }
    
    void Render()
    {
        // Draw player at (x, y)
    }
};
```

#### Après (nouveau système ECS)

```cpp
// ✅ Nouveau code ECS modulaire

// === COMPONENTS ===
struct Position_data { Vector position; };
struct Velocity_data { Vector velocity; };
struct PhysicsBody_data
{
    float gravity = 980.f;
    bool onGround = false;
};
struct PlayerController_data
{
    Vector Joydirection;
    bool isJumping = false;
};
struct PlayerBinding_data { short playerIndex; short controllerID; };

// === SYSTEMS ===

// 1. Input System
class PlatformerInputSystem : public ECS_System
{
public:
    PlatformerInputSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
    }
    
    void Process() override
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            pc.Joydirection.x = 0;
            if (keys[SDL_SCANCODE_A]) pc.Joydirection.x = -1;
            if (keys[SDL_SCANCODE_D]) pc.Joydirection.x = 1;
            
            pc.isJumping = keys[SDL_SCANCODE_SPACE];
        }
    }
};

// 2. Jump System
class JumpSystem : public ECS_System
{
public:
    JumpSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Velocity_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PhysicsBody_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            Velocity_data& vel = World::Get().GetComponent<Velocity_data>(entity);
            PhysicsBody_data& physics = World::Get().GetComponent<PhysicsBody_data>(entity);
            
            if (pc.isJumping && physics.onGround)
            {
                vel.velocity.y = -400.f; // Jump velocity
                physics.onGround = false;
            }
        }
    }
};

// 3. Physics System (gravity)
class GravitySystem : public ECS_System
{
public:
    GravitySystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Velocity_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PhysicsBody_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Velocity_data& vel = World::Get().GetComponent<Velocity_data>(entity);
            PhysicsBody_data& physics = World::Get().GetComponent<PhysicsBody_data>(entity);
            
            vel.velocity.y += physics.gravity * GameEngine::fDt;
        }
    }
};

// 4. Movement System
class PlatformerMovementSystem : public ECS_System
{
public:
    PlatformerMovementSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Velocity_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
    }
    
    void Process() override
    {
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Velocity_data& vel = World::Get().GetComponent<Velocity_data>(entity);
            PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
            
            // Horizontal movement
            vel.velocity.x = pc.Joydirection.x * 200.f;
            
            // Apply velocity
            pos.position.x += vel.velocity.x * GameEngine::fDt;
            pos.position.y += vel.velocity.y * GameEngine::fDt;
        }
    }
};

// 5. Ground Check System
class GroundCheckSystem : public ECS_System
{
public:
    GroundCheckSystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<Velocity_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PhysicsBody_data>(), true);
    }
    
    void Process() override
    {
        const float groundY = 400.f;
        
        for (EntityID entity : m_entities)
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Velocity_data& vel = World::Get().GetComponent<Velocity_data>(entity);
            PhysicsBody_data& physics = World::Get().GetComponent<PhysicsBody_data>(entity);
            
            if (pos.position.y >= groundY)
            {
                pos.position.y = groundY;
                vel.velocity.y = 0;
                physics.onGround = true;
            }
            else
            {
                physics.onGround = false;
            }
        }
    }
};

// === SETUP ===
void SetupPlatformerGame()
{
    // Create player entity
    EntityID player = World::Get().CreateEntity();
    
    Position_data pos;
    pos.position = Vector{100.f, 400.f, 0.f};
    World::Get().AddComponent<Position_data>(player, pos);
    
    World::Get().AddComponent<Velocity_data>(player);
    World::Get().AddComponent<PhysicsBody_data>(player);
    World::Get().AddComponent<PlayerController_data>(player);
    
    PlayerBinding_data binding;
    binding.playerIndex = 0;
    binding.controllerID = -1;
    World::Get().AddComponent<PlayerBinding_data>(player, binding);
    
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
    
    // Register systems (order matters!)
    World::Get().RegisterSystem(std::make_shared<PlatformerInputSystem>());
    World::Get().RegisterSystem(std::make_shared<JumpSystem>());
    World::Get().RegisterSystem(std::make_shared<GravitySystem>());
    World::Get().RegisterSystem(std::make_shared<PlatformerMovementSystem>());
    World::Get().RegisterSystem(std::make_shared<GroundCheckSystem>());
    
    std::cout << "✅ Platformer game setup complete!" << std::endl;
}
```

---

## Checklist de migration

### Phase 1 : Préparation

- [ ] **Analyser le code existant**
  - Identifier tous les endroits où les inputs sont lus
  - Lister toutes les actions gameplay (move, jump, shoot, etc.)
  - Noter les périphériques utilisés (keyboard, mouse, gamepad)

- [ ] **Créer les components nécessaires**
  - PlayerBinding_data
  - Controller_data
  - PlayerController_data
  - Position_data (ou équivalent existant)
  - Autres components gameplay

- [ ] **Identifier les systèmes à créer**
  - Système d'input (mapping hardware → gameplay)
  - Systèmes gameplay (mouvement, actions, etc.)

### Phase 2 : Migration du code

- [ ] **Remplacer les accès directs SDL**
  - ❌ Supprimer : `SDL_PollEvent` dans le gameplay
  - ✅ Ajouter : Systèmes ECS

- [ ] **Créer les entités joueurs**
  - Ajouter tous les components nécessaires
  - Bind les contrôleurs aux joueurs

- [ ] **Implémenter les systèmes**
  - Input system (lecture hardware)
  - Gameplay systems (logique métier)

- [ ] **Enregistrer les systèmes dans le World**
  - Attention à l'ordre d'exécution !

### Phase 3 : Tests

- [ ] **Tester chaque action**
  - Mouvement (4 directions, analogique)
  - Actions (jump, shoot, interact)
  - Hotplug (déconnexion/reconnexion)

- [ ] **Tester multi-joueur**
  - 2+ joueurs simultanés
  - Mix keyboard + gamepad

- [ ] **Tester les edge cases**
  - Appui simultané de touches opposées
  - Déconnexion pendant le gameplay
  - Changement rapide de contrôleur

### Phase 4 : Nettoyage

- [ ] **Supprimer l'ancien code**
  - Classes Player monolithiques
  - Accès directs SDL
  - Code dupliqué

- [ ] **Optimiser**
  - Vérifier l'ordre des systèmes
  - Supprimer les systems inutilisés
  - Profiler les performances

- [ ] **Documenter**
  - Commenter les nouveaux systèmes
  - Mettre à jour la documentation projet

---

## Problèmes courants

### Problème 1 : "Mes inputs ne marchent pas après migration"

**Symptômes :**
- Le joueur ne bouge plus
- Les actions ne fonctionnent pas

**Causes possibles :**

1. **Binding manquant**
   ```cpp
   // ❌ Oublié
   // InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
   
   // ✅ Fix
   InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
   ```

2. **Component manquant**
   ```cpp
   // ❌ Oublié PlayerBinding_data
   World::Get().AddComponent<Position_data>(player);
   World::Get().AddComponent<PlayerController_data>(player);
   
   // ✅ Fix : ajouter tous les components requis
   World::Get().AddComponent<Position_data>(player);
   World::Get().AddComponent<PlayerBinding_data>(player);
   World::Get().AddComponent<Controller_data>(player);
   World::Get().AddComponent<PlayerController_data>(player);
   ```

3. **Système non enregistré**
   ```cpp
   // ❌ Oublié d'enregistrer le système
   auto inputSystem = std::make_shared<MyInputSystem>();
   // World::Get().RegisterSystem(inputSystem); // Oublié !
   
   // ✅ Fix
   World::Get().RegisterSystem(inputSystem);
   ```

---

### Problème 2 : "Le système ne voit pas mes entités"

**Symptômes :**
- `m_entities.size() == 0` dans le système

**Cause : Signature incorrecte**

```cpp
// ❌ Mauvais : signature incomplète
class MySystem : public ECS_System
{
public:
    MySystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        // Oublié PlayerController_data !
    }
};

// ✅ Fix : signature complète
class MySystem : public ECS_System
{
public:
    MySystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
    }
};
```

---

### Problème 3 : "Ordre d'exécution incorrect"

**Symptômes :**
- Les inputs sont en retard d'une frame
- Les mouvements sont saccadés

**Cause : Ordre des systèmes**

```cpp
// ❌ Mauvais ordre
World::Get().RegisterSystem(std::make_shared<MovementSystem>());      // Exécuté en 1er
World::Get().RegisterSystem(std::make_shared<PlayerInputSystem>());   // Exécuté en 2ème

// Résultat : MovementSystem lit les inputs de la frame précédente !

// ✅ Fix : bon ordre
World::Get().RegisterSystem(std::make_shared<PlayerInputSystem>());   // 1. Lire inputs
World::Get().RegisterSystem(std::make_shared<MovementSystem>());      // 2. Appliquer mouvement
```

---

### Problème 4 : "Crash lors de l'accès aux components"

**Symptômes :**
- Exception `std::out_of_range`
- Segmentation fault

**Cause : Entité invalide ou component manquant**

```cpp
// ❌ Risqué : pas de vérification
PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);

// ✅ Fix : try/catch ou HasComponent
try
{
    PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
    // ...
}
catch (const std::exception& e)
{
    std::cerr << "Error: " << e.what() << std::endl;
}

// Ou :
if (World::Get().HasComponent<PlayerController_data>(entity))
{
    PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
    // ...
}
```

---

## Support

**Questions ?**
- 📖 Consulter [INPUT_SYSTEM_GUIDE.md](INPUT_SYSTEM_GUIDE.md)
- 🔧 Voir [INPUT_API_REFERENCE.md](INPUT_API_REFERENCE.md)
- 🐛 Consulter [INPUT_TROUBLESHOOTING.md](INPUT_TROUBLESHOOTING.md)

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
