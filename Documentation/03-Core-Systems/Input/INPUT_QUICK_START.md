# 🚀 Input System - Quick Start Guide

> **Objectif :** Créer une entité contrôlable et lire ses inputs en moins de 10 minutes !

---

## Table des matières

- [Prérequis](#prérequis)
- [Étape 1 : Créer une entité contrôlable](#étape-1--créer-une-entité-contrôlable)
- [Étape 2 : Lire les inputs dans un système](#étape-2--lire-les-inputs-dans-un-système)
- [Étape 3 : Tester avec plusieurs joueurs](#étape-3--tester-avec-plusieurs-joueurs)
- [Étape 4 : Gestion des contextes (UI/Gameplay)](#étape-4--gestion-des-contextes-uigameplay)
- [Next Steps](#next-steps)

---

## Prérequis

Assurez-vous que :
- ✅ SDL3 est initialisé
- ✅ Le `World` ECS est initialisé
- ✅ Les managers d'input sont initialisés :
  ```cpp
  KeyboardManager::Get().Initialize();
  JoystickManager::Get().Initialize();
  MouseManager::Get().Initialize();
  ```

---

## Étape 1 : Créer une entité contrôlable (5 min)

Voici le code complet pour créer une entité que vous pouvez contrôler avec le clavier :

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "InputsManager.h"

void CreatePlayerEntity()
{
    // 1. Créer l'entité
    EntityID player = World::Get().CreateEntity();
    
    // 2. Ajouter un composant Position (pour savoir où est le joueur)
    Position_data pos;
    pos.position = Vector{100.f, 100.f, 0.f}; // Position initiale
    World::Get().AddComponent<Position_data>(player, pos);
    
    // 3. Ajouter PlayerBinding (lie le joueur à un device)
    PlayerBinding_data binding;
    binding.playerIndex = 0;      // Joueur 0
    binding.controllerID = -1;    // -1 = clavier (>= 0 = joystick ID)
    World::Get().AddComponent<PlayerBinding_data>(player, binding);
    
    // 4. Ajouter Controller (état hardware du contrôleur)
    Controller_data ctrl;
    ctrl.controllerID = -1;       // -1 = clavier
    ctrl.isConnected = true;      // Marquer comme connecté
    World::Get().AddComponent<Controller_data>(player, ctrl);
    
    // 5. Ajouter PlayerController (état gameplay : directions, actions)
    PlayerController_data playerCtrl;
    playerCtrl.Joydirection = Vector{0.f, 0.f, 0.f}; // Pas de mouvement au départ
    World::Get().AddComponent<PlayerController_data>(player, playerCtrl);
    
    // 6. Bind le contrôleur au joueur dans InputsManager
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1)); // Bind keyboard to player 0
    
    std::cout << "✅ Player entity created with ID: " << player << std::endl;
}
```

### Explication ligne par ligne

| Ligne | Explication |
|-------|-------------|
| `CreateEntity()` | Crée une nouvelle entité ECS (retourne un ID unique) |
| `Position_data` | Stocke la position 2D/3D de l'entité |
| `PlayerBinding_data` | Lie le joueur 0 à un device (-1 = keyboard) |
| `Controller_data` | État hardware brut du contrôleur (boutons, connexion) |
| `PlayerController_data` | État gameplay (direction, actions) |
| `BindControllerToPlayer()` | Enregistre le binding dans InputsManager |

---

## Étape 2 : Lire les inputs dans un système (3 min)

Maintenant, créons un système ECS qui lit les inputs et déplace le joueur :

```cpp
#include "ECS_Systems.h"
#include "ECS_Components.h"
#include "World.h"
#include "GameEngine.h"

class PlayerMovementSystem : public ECS_System
{
public:
    PlayerMovementSystem()
    {
        // Définir les composants requis pour ce système
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
    }
    
    void Process() override
    {
        // Itérer sur toutes les entités qui ont Position + PlayerController
        for (EntityID entity : m_entities)
        {
            try
            {
                // Récupérer les composants
                Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                PlayerController_data& pc = World::Get().GetComponent<PlayerController_data>(entity);
                
                // ✅ Lire la direction du joystick/clavier
                Vector direction = pc.Joydirection;
                
                // ✅ Appliquer le mouvement (150 pixels/seconde)
                float speed = 150.f;
                pos.position += direction * speed * GameEngine::fDt; // fDt = delta time
                
                // ✅ Gérer les actions (optionnel)
                if (pc.isJumping)
                {
                    // Logique de saut
                    std::cout << "🎮 Player is jumping!" << std::endl;
                }
                
                if (pc.isShooting)
                {
                    // Logique de tir
                    std::cout << "🎮 Player is shooting!" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "❌ Error in PlayerMovementSystem: " << e.what() << std::endl;
            }
        }
    }
};
```

### Enregistrer le système dans le World

```cpp
// Dans votre fonction d'initialisation du jeu
void InitializeGame()
{
    // Créer et enregistrer le système
    auto movementSystem = std::make_shared<PlayerMovementSystem>();
    World::Get().RegisterSystem(movementSystem);
    
    std::cout << "✅ PlayerMovementSystem registered" << std::endl;
}
```

### Comment ça marche ?

1. **PlayerControlSystem** (built-in) lit les événements hardware et remplit `PlayerController_data.Joydirection`
2. **Votre système** lit `PlayerController_data.Joydirection` et déplace `Position_data`
3. **RenderingSystem** (built-in) affiche l'entité à la nouvelle position

```
Keyboard Press (W)
    │
    ▼
PlayerControlSystem
    │ (écrit dans PlayerController_data)
    ▼
PlayerMovementSystem ← Votre code
    │ (lit PlayerController_data, écrit dans Position_data)
    ▼
RenderingSystem
    │ (lit Position_data, affiche à l'écran)
```

---

## Étape 3 : Tester avec plusieurs joueurs (2 min)

Ajoutons un deuxième joueur avec une manette :

```cpp
void CreateMultiplayerSetup()
{
    // ========== Joueur 1 : Clavier ==========
    EntityID player1 = World::Get().CreateEntity();
    
    Position_data pos1;
    pos1.position = Vector{100.f, 100.f, 0.f};
    World::Get().AddComponent<Position_data>(player1, pos1);
    
    PlayerBinding_data binding1;
    binding1.playerIndex = 0;
    binding1.controllerID = -1; // Keyboard
    World::Get().AddComponent<PlayerBinding_data>(player1, binding1);
    
    Controller_data ctrl1;
    ctrl1.controllerID = -1;
    ctrl1.isConnected = true;
    World::Get().AddComponent<Controller_data>(player1, ctrl1);
    
    World::Get().AddComponent<PlayerController_data>(player1);
    
    InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
    
    std::cout << "✅ Player 1 (Keyboard) created" << std::endl;
    
    // ========== Joueur 2 : Manette ==========
    
    // Vérifier qu'une manette est connectée
    auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
    if (joysticks.empty())
    {
        std::cerr << "⚠️ No joystick connected for Player 2!" << std::endl;
        return;
    }
    
    SDL_JoystickID joystickID = joysticks[0]; // Première manette
    
    EntityID player2 = World::Get().CreateEntity();
    
    Position_data pos2;
    pos2.position = Vector{300.f, 100.f, 0.f}; // Position différente
    World::Get().AddComponent<Position_data>(player2, pos2);
    
    PlayerBinding_data binding2;
    binding2.playerIndex = 1;           // Joueur 1
    binding2.controllerID = joystickID; // Manette connectée
    World::Get().AddComponent<PlayerBinding_data>(player2, binding2);
    
    Controller_data ctrl2;
    ctrl2.controllerID = joystickID;
    ctrl2.isConnected = true;
    World::Get().AddComponent<Controller_data>(player2, ctrl2);
    
    World::Get().AddComponent<PlayerController_data>(player2);
    
    InputsManager::Get().BindControllerToPlayer(1, joystickID);
    
    std::cout << "✅ Player 2 (Joystick " << joystickID << ") created" << std::endl;
}
```

### Gestion automatique (hotplug)

Le système gère automatiquement la reconnexion des manettes :

```cpp
// Si une manette se déconnecte puis se reconnecte,
// le système tente de la re-binder au même joueur automatiquement
// (géré dans InputsManager::OnEvent())
```

---

## Étape 4 : Gestion des contextes (UI/Gameplay)

⚠️ **Note :** La gestion des contextes (UI vs Gameplay) n'est pas encore implémentée dans la version actuelle.

**Comportement attendu (futur) :**

```cpp
// Quand vous ouvrez un menu, bloquer les inputs gameplay
void OpenMenu()
{
    // InputsManager::Get().PushContext(InputContext::UI);
    // Les inputs gameplay (PlayerController) sont bloqués
    // Seuls les inputs UI (navigation menu) fonctionnent
}

void CloseMenu()
{
    // InputsManager::Get().PopContext();
    // Retour au contexte gameplay
}
```

**Solution temporaire :**

```cpp
// Utiliser un flag global
bool isInMenu = false;

void PlayerMovementSystem::Process()
{
    if (isInMenu) return; // Skip gameplay logic
    
    // ... reste du code
}
```

---

## Next Steps

Vous savez maintenant créer une entité contrôlable et lire ses inputs ! 🎉

**Pour aller plus loin :**

- 📖 [System Guide](INPUT_SYSTEM_GUIDE.md) - Comprendre l'architecture complète
- 🔧 [API Reference](INPUT_API_REFERENCE.md) - Documentation détaillée de l'API
- 💡 [Examples](INPUT_EXAMPLES.md) - Exemples avancés (rebinding, deadzone, etc.)
- 🏗️ [Architecture](INPUT_ARCHITECTURE.md) - Décisions de design et optimisations

---

## Troubleshooting rapide

### ❌ "Mes inputs ne fonctionnent pas"

**Checklist :**
1. ✅ Les managers sont initialisés ?
   ```cpp
   KeyboardManager::Get().Initialize();
   JoystickManager::Get().Initialize();
   ```
2. ✅ `InputsManager::HandleEvent()` est appelé dans la boucle SDL ?
   ```cpp
   while (SDL_PollEvent(&event)) {
       InputsManager::Get().HandleEvent(&event);
   }
   ```
3. ✅ Le binding est correctement fait ?
   ```cpp
   InputsManager::Get().BindControllerToPlayer(0, SDL_JoystickID(-1));
   ```
4. ✅ Le `PlayerControlSystem` est enregistré et s'exécute ?
   ```cpp
   World::Get().RegisterSystem(std::make_shared<PlayerControlSystem>());
   ```

### ❌ "La manette n'est pas détectée"

```cpp
// Vérifier les manettes connectées
auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
std::cout << "Connected joysticks: " << joysticks.size() << std::endl;
for (auto jid : joysticks) {
    std::cout << "  - Joystick ID: " << jid << std::endl;
}
```

### ❌ "Les inputs sont en retard (lag)"

- Assurez-vous que `PlayerControlSystem::Process()` s'exécute **avant** les systèmes gameplay
- Vérifiez que `EventManager::Process()` est appelé chaque frame

---

**Félicitations ! Vous êtes prêt à créer des jeux avec le système Input ECS ! 🎮**

---

**Dernière mise à jour :** 2025  
**Olympe Engine V2**
