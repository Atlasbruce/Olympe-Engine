# 🐛 Input System - Dépannage

> **Guide de résolution des problèmes courants du système Input ECS.**

---

## Table des matières

- [Problèmes d'input](#problèmes-dinput)
- [Problèmes de manette](#problèmes-de-manette)
- [Problèmes de performance](#problèmes-de-performance)
- [Problèmes ECS](#problèmes-ecs)
- [Outils de debug](#outils-de-debug)
- [Logs et diagnostics](#logs-et-diagnostics)

---

## Problèmes d'input

### ❌ "Mes inputs ne marchent pas"

#### Symptômes
- Aucune réponse aux touches clavier
- Le joueur ne bouge pas
- Les actions ne se déclenchent pas

#### Checklist de diagnostic

**1. Vérifier que les managers sont initialisés**

```cpp
// Dans votre fonction d'initialisation
KeyboardManager::Get().Initialize();
JoystickManager::Get().Initialize();
MouseManager::Get().Initialize();
```

**Test :**
```cpp
// Vérifier dans les logs
// Devrait afficher: "KeyboardManager created and Initialized"
// Devrait afficher: "JoystickManager created and Initialized with X joysticks connected"
```

---

**2. Vérifier que HandleEvent() est appelé**

```cpp
// Dans votre boucle principale
SDL_Event event;
while (SDL_PollEvent(&event))
{
    InputsManager::Get().HandleEvent(&event); // ✅ DOIT être appelé
    // ... autres handlers
}
```

**Test :**
```cpp
// Ajouter un log temporaire dans InputsManager::HandleEvent()
void InputsManager::HandleEvent(const SDL_Event* ev)
{
    std::cout << "Event type: " << ev->type << std::endl; // DEBUG
    // ...
}
```

---

**3. Vérifier le binding joueur ↔ contrôleur**

```cpp
// Vérifier si le binding existe
if (!InputsManager::Get().IsPlayerBound(0))
{
    std::cerr << "❌ Player 0 not bound!" << std::endl;
    // Fix:
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
}
```

**Test :**
```cpp
// Afficher le status
std::string status = InputsManager::Get().GetDevicesStatusUpdate();
std::cout << status << std::endl;
```

---

**4. Vérifier que l'entité a tous les components requis**

```cpp
// Components nécessaires pour les inputs
bool hasPosition = World::Get().HasComponent<Position_data>(playerEntity);
bool hasBinding = World::Get().HasComponent<PlayerBinding_data>(playerEntity);
bool hasController = World::Get().HasComponent<Controller_data>(playerEntity);
bool hasPlayerCtrl = World::Get().HasComponent<PlayerController_data>(playerEntity);

if (!hasPosition || !hasBinding || !hasController || !hasPlayerCtrl)
{
    std::cerr << "❌ Missing components!" << std::endl;
    std::cerr << "  Position: " << hasPosition << std::endl;
    std::cerr << "  Binding: " << hasBinding << std::endl;
    std::cerr << "  Controller: " << hasController << std::endl;
    std::cerr << "  PlayerController: " << hasPlayerCtrl << std::endl;
}
```

---

**5. Vérifier que les systèmes sont enregistrés et s'exécutent**

```cpp
// Ajouter un log dans votre système
void MyInputSystem::Process()
{
    std::cout << "MyInputSystem::Process() - entities: " << m_entities.size() << std::endl;
    // ...
}
```

Si `m_entities.size() == 0` → Problème de signature (voir section ECS)

---

### ❌ "Les inputs sont en retard (lag)"

#### Symptômes
- Délai perceptible entre l'appui et la réaction
- Mouvements saccadés

#### Causes et solutions

**1. Ordre des systèmes incorrect**

```cpp
// ❌ Mauvais ordre
World::Get().RegisterSystem(std::make_shared<MovementSystem>());     // Exécuté en 1er
World::Get().RegisterSystem(std::make_shared<PlayerInputSystem>()); // Exécuté en 2ème

// Résultat: MovementSystem lit les inputs de la frame N-1

// ✅ Bon ordre
World::Get().RegisterSystem(std::make_shared<PlayerInputSystem>()); // 1. Lire inputs
World::Get().RegisterSystem(std::make_shared<MovementSystem>());     // 2. Appliquer
```

---

**2. EventManager::Process() pas appelé**

```cpp
// Dans votre boucle principale
void GameLoop()
{
    // 1. Poll SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        InputsManager::Get().HandleEvent(&event);
    }
    
    // 2. Process event queue ✅ IMPORTANT
    EventManager::Get().Process(); // Dispatch messages to listeners
    
    // 3. Process systems
    World::Get().ProcessSystems();
}
```

---

**3. Framerate trop bas**

```cpp
// Vérifier le framerate
std::cout << "FPS: " << (1.0f / GameEngine::fDt) << std::endl;

// Si FPS < 30, le lag est visible
// Solutions:
// - Optimiser le rendu
// - Réduire la complexité des systèmes
// - Profiler le code (voir section Performance)
```

---

### ❌ "Les inputs sont dupliqués"

#### Symptômes
- Action se déclenche plusieurs fois pour un seul appui
- Mouvement deux fois plus rapide

#### Causes et solutions

**1. Système appelé plusieurs fois**

```cpp
// ❌ Enregistré deux fois par erreur
World::Get().RegisterSystem(std::make_shared<MyInputSystem>());
World::Get().RegisterSystem(std::make_shared<MyInputSystem>()); // Doublon !

// ✅ Fix: enregistrer une seule fois
auto inputSystem = std::make_shared<MyInputSystem>();
World::Get().RegisterSystem(inputSystem);
```

---

**2. Plusieurs entités reçoivent les mêmes inputs**

```cpp
// Diagnostic: afficher les entités traitées
void MyInputSystem::Process()
{
    std::cout << "Processing " << m_entities.size() << " entities" << std::endl;
    for (EntityID entity : m_entities)
    {
        std::cout << "  - Entity " << entity << std::endl;
    }
}

// Si plusieurs entités sont listées alors qu'il ne devrait y en avoir qu'une:
// → Vérifier que vous ne créez pas de doublons
// → Utiliser PlayerBinding_data.playerIndex pour filtrer
```

**Solution : Filtrer par playerIndex**

```cpp
void MyInputSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
        
        // Ne traiter que le joueur 0
        if (binding.playerIndex != 0)
            continue;
        
        // ...
    }
}
```

---

## Problèmes de manette

### ❌ "Le gamepad n'est pas détecté"

#### Symptômes
- `GetConnectedJoysticks()` retourne une liste vide
- Impossible de binder une manette

#### Checklist de diagnostic

**1. Vérifier que SDL est initialisé correctement**

```cpp
// SDL3 initialization
if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD))
{
    std::cerr << "❌ SDL Init failed: " << SDL_GetError() << std::endl;
}
else
{
    std::cout << "✅ SDL initialized" << std::endl;
}
```

---

**2. Vérifier que JoystickManager est initialisé**

```cpp
JoystickManager::Get().Initialize();

// Devrait afficher:
// "JoystickManager created and Initialized with X joysticks connected"
```

---

**3. Scanner manuellement les manettes**

```cpp
JoystickManager::Get().Scan_Joysticks();

auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
std::cout << "Connected joysticks: " << joysticks.size() << std::endl;
for (auto jid : joysticks)
{
    std::cout << "  - Joystick ID: " << jid << std::endl;
}
```

---

**4. Vérifier avec SDL directement (bypass Olympe)**

```cpp
// Test direct SDL3
int count = 0;
SDL_JoystickID* ids = SDL_GetJoysticks(&count);
std::cout << "SDL3 reports " << count << " joystick(s)" << std::endl;
if (ids)
{
    for (int i = 0; i < count; ++i)
    {
        std::cout << "  - ID: " << ids[i] << std::endl;
    }
    SDL_free(ids);
}
```

Si SDL3 ne détecte rien → Problème hardware/driver, pas Olympe.

---

**5. Logs SDL détaillés**

```cpp
// Activer les logs SDL
SDL_SetLogPriority(SDL_LOG_CATEGORY_INPUT, SDL_LOG_PRIORITY_VERBOSE);
```

---

### ❌ "La manette se déconnecte sans raison"

#### Symptômes
- Manette fonctionnelle puis soudainement plus d'inputs
- `isConnected` devient `false`

#### Causes possibles

**1. Manette USB défectueuse**
- Essayer un autre câble
- Essayer un autre port USB

**2. Événement de déconnexion non géré**

```cpp
// Vérifier les logs pour:
// "Joystick removed id=X"

// Le système devrait automatiquement gérer la reconnexion
// Si ce n'est pas le cas, vérifier InputsManager::OnEvent()
```

---

### ❌ "Les axes sont inversés / deadzone incorrecte"

#### Symptômes
- Le stick pousse vers le bas alors qu'on pousse vers le haut
- Le personnage bouge alors que le stick est au repos

#### Solution : Ajuster le mapping et la deadzone

```cpp
class GamepadInputSystem : public ECS_System
{
    void Process() override
    {
        SDL_Joystick* joy = SDL_GetJoystickFromID(joystickID);
        
        // Lire les axes
        float x = SDL_GetJoystickAxis(joy, 0) / 32767.f;
        float y = SDL_GetJoystickAxis(joy, 1) / 32767.f;
        
        // ✅ Inverser Y si nécessaire
        y = -y; // Inversion pour que "haut" = négatif
        
        // ✅ Ajuster la deadzone
        const float deadzone = 0.15f; // Augmenter si drift
        if (std::abs(x) < deadzone) x = 0.f;
        if (std::abs(y) < deadzone) y = 0.f;
        
        // ✅ Appliquer une courbe (optionnel)
        // Pour rendre les petits mouvements plus précis
        x = std::pow(std::abs(x), 1.5f) * (x < 0 ? -1 : 1);
        y = std::pow(std::abs(y), 1.5f) * (y < 0 ? -1 : 1);
        
        pc.Joydirection.x = x;
        pc.Joydirection.y = y;
    }
};
```

---

## Problèmes de performance

### ❌ "Le jeu lag quand il y a beaucoup d'inputs"

#### Symptômes
- FPS drop quand on appuie sur plusieurs touches
- Lag perceptible avec plusieurs manettes

#### Profiling

**1. Mesurer le temps d'exécution des systèmes**

```cpp
class ProfiledSystem : public ECS_System
{
    void Process() override
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        // ... logique du système
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "System took " << duration.count() << " µs" << std::endl;
    }
};
```

---

**2. Identifier les bottlenecks**

```cpp
// Temps typiques attendus (pour référence):
// PlayerInputSystem: < 100 µs (0.1 ms)
// MovementSystem: < 200 µs (0.2 ms)
// RenderingSystem: < 5000 µs (5 ms)

// Si un système prend beaucoup plus:
// → Optimiser la logique
// → Réduire les allocations mémoire
// → Éviter les copies inutiles
```

---

**3. Optimisations possibles**

```cpp
// ❌ Mauvais: allocation à chaque frame
void Process()
{
    std::vector<EntityID> nearbyEntities; // Allocation !
    for (EntityID e : m_entities)
    {
        nearbyEntities.push_back(e);
    }
}

// ✅ Bon: membre de classe
class MySystem : public ECS_System
{
    std::vector<EntityID> m_nearbyEntities; // Alloué une fois
    
    void Process() override
    {
        m_nearbyEntities.clear(); // Pas d'allocation
        for (EntityID e : m_entities)
        {
            m_nearbyEntities.push_back(e);
        }
    }
};
```

---

## Problèmes ECS

### ❌ "Mon système ne voit aucune entité (m_entities vide)"

#### Symptômes
- `m_entities.size() == 0` dans le système
- Le système ne s'exécute jamais vraiment

#### Cause : Signature incorrecte

**Diagnostic :**

```cpp
class MySystem : public ECS_System
{
public:
    MySystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
        
        // DEBUG: afficher la signature
        std::cout << "MySystem signature: " << requiredSignature << std::endl;
    }
    
    void Process() override
    {
        std::cout << "MySystem::Process() - " << m_entities.size() << " entities" << std::endl;
        
        if (m_entities.empty())
        {
            std::cerr << "⚠️ No entities match the signature!" << std::endl;
        }
    }
};
```

---

**Solution : Vérifier que l'entité a TOUS les components requis**

```cpp
// Créer l'entité avec TOUS les components de la signature
EntityID player = World::Get().CreateEntity();

// Signature requiert Position_data
World::Get().AddComponent<Position_data>(player);

// Signature requiert PlayerController_data
World::Get().AddComponent<PlayerController_data>(player);

// ✅ Maintenant l'entité correspond à la signature
```

---

### ❌ "Exception lors de l'accès à un component"

#### Symptômes
- `std::out_of_range` exception
- Crash lors de `GetComponent<T>()`

#### Causes et solutions

**1. L'entité n'a pas le component**

```cpp
// ❌ Dangereux: pas de vérification
PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);

// ✅ Sécurisé: vérifier avant
if (World::Get().HasComponent<PlayerController_data>(entity))
{
    PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
}
else
{
    std::cerr << "❌ Entity " << entity << " has no PlayerController_data" << std::endl;
}

// ✅ Ou: try/catch
try
{
    PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
}
catch (const std::exception& e)
{
    std::cerr << "❌ Error: " << e.what() << std::endl;
}
```

---

**2. L'entité a été détruite**

```cpp
// ❌ Stocker un EntityID long-terme sans vérifier
EntityID player = CreatePlayer();
// ... beaucoup de code ...
World::Get().DestroyEntity(player); // Détruit
// ... plus tard ...
auto& pc = World::Get().GetComponent<PlayerController_data>(player); // ❌ Crash !

// ✅ Vérifier que l'entité existe
if (World::Get().IsEntityValid(player))
{
    auto& pc = World::Get().GetComponent<PlayerController_data>(player);
}
```

---

## Outils de debug

### 1. Inspector Panel (ImGui)

Si vous avez un panel ImGui pour les inputs :

```cpp
// Dans InputsInspectorPanel.cpp
void InputsInspectorPanel::Render()
{
    ImGui::Begin("Input Inspector");
    
    // Device status
    std::string status = InputsManager::Get().GetDevicesStatusUpdate();
    ImGui::TextUnformatted(status.c_str());
    
    // Player bindings
    ImGui::Separator();
    ImGui::Text("Player Bindings:");
    for (int i = 0; i < 4; ++i)
    {
        if (InputsManager::Get().IsPlayerBound(i))
        {
            SDL_JoystickID jid = InputsManager::Get().GetPlayerBinding(i);
            ImGui::Text("  Player %d → Controller %d", i, jid);
        }
    }
    
    ImGui::End();
}
```

---

### 2. Console de debug pour les inputs

```cpp
class InputDebugConsole
{
public:
    static void LogInput(const std::string& msg)
    {
        m_logs.push_back(msg);
        if (m_logs.size() > 100) // Garder les 100 derniers
            m_logs.erase(m_logs.begin());
    }
    
    static void Render()
    {
        ImGui::Begin("Input Debug Console");
        for (const auto& log : m_logs)
        {
            ImGui::TextUnformatted(log.c_str());
        }
        ImGui::End();
    }
    
private:
    static std::vector<std::string> m_logs;
};

// Usage dans votre système
void MyInputSystem::Process()
{
    InputDebugConsole::LogInput("Processing inputs...");
    // ...
}
```

---

### 3. Visualisation des components

```cpp
void DebugDrawEntity(EntityID entity)
{
    std::cout << "=== Entity " << entity << " ===" << std::endl;
    
    if (World::Get().HasComponent<Position_data>(entity))
    {
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        std::cout << "  Position: " << pos.position.x << ", " << pos.position.y << std::endl;
    }
    
    if (World::Get().HasComponent<PlayerController_data>(entity))
    {
        PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
        std::cout << "  Direction: " << pc.Joydirection.x << ", " << pc.Joydirection.y << std::endl;
        std::cout << "  Jumping: " << pc.isJumping << std::endl;
        std::cout << "  Shooting: " << pc.isShooting << std::endl;
    }
    
    if (World::Get().HasComponent<PlayerBinding_data>(entity))
    {
        PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
        std::cout << "  Binding: Player " << binding.playerIndex 
                  << " → Controller " << binding.controllerID << std::endl;
    }
    
    std::cout << "===================" << std::endl;
}
```

---

## Logs et diagnostics

### Activer les logs du système Input

```cpp
// Dans votre code d'initialisation
#define DEBUG_INPUTS 1

#if DEBUG_INPUTS
    #define INPUT_LOG(x) std::cout << "[INPUT] " << x << std::endl
#else
    #define INPUT_LOG(x)
#endif

// Usage
INPUT_LOG("Player 0 bound to keyboard");
INPUT_LOG("Joystick " << jid << " connected");
```

---

### Logs SDL

```cpp
// Activer les logs SDL pour les inputs
SDL_SetLogPriority(SDL_LOG_CATEGORY_INPUT, SDL_LOG_PRIORITY_DEBUG);

// Autres catégories utiles:
SDL_SetLogPriority(SDL_LOG_CATEGORY_SYSTEM, SDL_LOG_PRIORITY_INFO);
SDL_SetLogPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_WARN);
```

---

### Dump de l'état complet

```cpp
void DumpInputState()
{
    std::cout << "\n========== INPUT STATE DUMP ==========\n";
    
    // Devices
    std::cout << "Connected joysticks: " 
              << InputsManager::Get().GetConnectedJoysticksCount() << "\n";
    std::cout << "Keyboard assigned: " 
              << (InputsManager::Get().IsKeyboardAssigned() ? "Yes" : "No") << "\n";
    
    // Bindings
    std::cout << "\nPlayer bindings:\n";
    for (int i = 0; i < 4; ++i)
    {
        if (InputsManager::Get().IsPlayerBound(i))
        {
            SDL_JoystickID jid = InputsManager::Get().GetPlayerBinding(i);
            std::cout << "  Player " << i << " → Controller " << jid << "\n";
        }
    }
    
    // Keyboard state
    std::cout << "\nKeyboard state (selected keys):\n";
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_W]) std::cout << "  W pressed\n";
    if (keys[SDL_SCANCODE_A]) std::cout << "  A pressed\n";
    if (keys[SDL_SCANCODE_S]) std::cout << "  S pressed\n";
    if (keys[SDL_SCANCODE_D]) std::cout << "  D pressed\n";
    if (keys[SDL_SCANCODE_SPACE]) std::cout << "  SPACE pressed\n";
    
    std::cout << "======================================\n\n";
}

// Appeler avec une touche de debug (F3)
if (keys[SDL_SCANCODE_F3])
{
    DumpInputState();
}
```

---

## Résumé des checks rapides

### Checklist rapide (en cas de problème)

```
[ ] SDL3 initialisé avec SDL_INIT_JOYSTICK
[ ] Managers initialisés (Keyboard, Joystick, Mouse, Inputs)
[ ] HandleEvent() appelé dans la boucle SDL
[ ] EventManager::Process() appelé chaque frame
[ ] Joueur bindé à un contrôleur
[ ] Entité a tous les components requis
[ ] Système enregistré dans le World
[ ] Ordre des systèmes correct
[ ] Signature du système correspond aux components
```

---

## Support

**Besoin d'aide ?**
- 📖 [System Guide](INPUT_SYSTEM_GUIDE.md) - Vue d'ensemble
- 🚀 [Quick Start](INPUT_QUICK_START.md) - Guide rapide
- 🔧 [API Reference](INPUT_API_REFERENCE.md) - Documentation API
- 💡 [Examples](INPUT_EXAMPLES.md) - Exemples pratiques
- 🏗️ [Architecture](INPUT_ARCHITECTURE.md) - Design détaillé
- 🔄 [Migration](INPUT_MIGRATION.md) - Migration depuis ancien code

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
