# EventQueue ECS System - Guide d'utilisation

## Vue d'ensemble

Le système **EventQueue ECS** remplace les événements ad hoc et les bus de messages génériques par un système d'événements typés, découplés et intégrés à l'architecture ECS. Ce système permet une meilleure gestion de l'état, facilite le débogage, et permet le replay/test.

## Architecture

### Composants principaux

1. **Event** - Structure d'événement typée avec union discriminée
2. **EventQueue_data** - Composant avec ring buffer pour stockage d'événements
3. **EventQueueSystem** - Système ECS pour gestion globale des événements
4. **Types d'événements typés** - Structures définies pour chaque catégorie d'événement

### Types d'événements disponibles

#### Événements d'entrée (Input Events)
- `InputButtonEvent` - Pression/relâchement de bouton
- `InputAxisEvent` - Mouvement d'axe analogique
- `InputActionEvent` - Action gameplay abstraite

#### Événements de cycle de vie d'entité (Entity Lifecycle)
- `EntityJoinedEvent` - Joueur rejoint le jeu
- `EntityLeftEvent` - Joueur quitte le jeu
- `EntitySpawnedEvent` - Entité créée
- `EntityDestroyedEvent` - Entité détruite

#### Événements de gameplay
- `CollisionEvent` - Collision entre entités
- `TriggerEvent` - Entrée/sortie de zone de déclenchement
- `HealthChangedEvent` - Changement de points de vie
- `ItemCollectedEvent` - Collecte d'objet

#### Événements système
- `LevelEvent` - Chargement/déchargement de niveau
- `GameStateEvent` - Pause/reprise du jeu

## Utilisation

### 1. Poster un événement

```cpp
// Obtenir le système EventQueue
EventQueueSystem* eventQueue = World::Get().GetEventQueueSystem();

// Créer un événement
InputButtonEvent btnEvt;
btnEvt.sourceEntity = playerEntity;
btnEvt.deviceId = 0;
btnEvt.buttonId = 2;
btnEvt.pressed = true;
btnEvt.timestamp = SDL_GetTicks() / 1000.0f;

// Poster l'événement
Event evt = Event::CreateInputButton(btnEvt);
eventQueue->PostEvent(ECSEventType::InputButton, evt);
```

### 2. Consommer des événements

Dans un système ECS, consommez les événements pendant `Process()`:

```cpp
void MySystem::Process()
{
    EventQueueSystem* eventQueue = World::Get().GetEventQueueSystem();
    
    // Consommer tous les événements de collision
    eventQueue->ConsumeEvents(ECSEventType::CollisionStarted, 
        [this](const Event& evt) {
            const CollisionEvent& collision = evt.data.collision;
            
            // Traiter la collision
            if (collision.entityA == myTrackedEntity) {
                HandleCollision(collision);
            }
        });
}
```

### 3. Observer sans consommer (Peek)

```cpp
// Lire les événements sans les retirer de la queue
eventQueue->PeekEvents(ECSEventType::TriggerEntered,
    [](const Event& evt) {
        const TriggerEvent& trigger = evt.data.trigger;
        LogTrigger(trigger);
    });
```

### 4. Gestion de la queue

```cpp
// Obtenir le nombre d'événements en attente
size_t count = eventQueue->GetEventCount(ECSEventType::InputButton);

// Effacer tous les événements d'un type spécifique
eventQueue->ClearEvents(ECSEventType::InputAxis);

// Effacer tous les événements
eventQueue->ClearAllEvents();
```

## EventQueue_data Component

Le composant `EventQueue_data` peut être attaché à des entités individuelles pour créer des queues d'événements locales:

```cpp
// Ajouter une queue d'événements à une entité
EntityID entity = World::Get().CreateEntity();
EventQueue_data& queue = World::Get().AddComponent<EventQueue_data>(entity);

// Poster un événement dans la queue de l'entité
CollisionEvent collision;
collision.entityA = entity;
collision.entityB = otherEntity;
collision.started = true;

Event evt = Event::CreateCollision(collision);
queue.PushEvent(evt);

// Consommer les événements de l'entité
Event evt;
while (queue.PopEvent(evt)) {
    if (evt.type == ECSEventType::CollisionStarted) {
        // Traiter l'événement
    }
}
```

### API EventQueue_data

```cpp
// Capacité du ring buffer
static constexpr size_t QUEUE_CAPACITY = 64;

// Ajouter un événement (retourne false si plein)
bool PushEvent(const Event& evt);

// Retirer et obtenir un événement (retourne false si vide)
bool PopEvent(Event& outEvt);

// Observer le prochain événement sans le retirer
bool PeekEvent(Event& outEvt) const;

// Vérifier l'état de la queue
bool IsEmpty() const;
bool IsFull() const;
size_t GetCount() const;

// Effacer tous les événements
void Clear();
```

## Exemples d'utilisation

### Exemple 1: Système de collision

```cpp
class CollisionSystem : public ECS_System
{
public:
    CollisionSystem() {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<BoundingBox_data>(), true);
    }
    
    void Process() override
    {
        EventQueueSystem* eventQueue = World::Get().GetEventQueueSystem();
        
        // Détecter les collisions
        for (EntityID entityA : m_entities) {
            for (EntityID entityB : m_entities) {
                if (entityA >= entityB) continue;
                
                if (CheckCollision(entityA, entityB)) {
                    // Créer un événement de collision
                    CollisionEvent collision;
                    collision.entityA = entityA;
                    collision.entityB = entityB;
                    collision.started = true;
                    collision.timestamp = SDL_GetTicks() / 1000.0f;
                    
                    // Poster l'événement
                    Event evt = Event::CreateCollision(collision);
                    eventQueue->PostEvent(ECSEventType::CollisionStarted, evt);
                }
            }
        }
    }
};
```

### Exemple 2: Système de dommages

```cpp
class DamageSystem : public ECS_System
{
public:
    DamageSystem() {
        requiredSignature.set(GetComponentTypeID_Static<Health_data>(), true);
    }
    
    void Process() override
    {
        EventQueueSystem* eventQueue = World::Get().GetEventQueueSystem();
        
        // Traiter les événements de collision pour appliquer des dommages
        eventQueue->ConsumeEvents(ECSEventType::CollisionStarted,
            [this](const Event& evt) {
                const CollisionEvent& collision = evt.data.collision;
                
                // Vérifier si une entité a des dommages
                ApplyDamageIfNeeded(collision.entityA, collision.entityB);
            });
        
        // Consommer les événements de santé pour notification
        eventQueue->ConsumeEvents(ECSEventType::HealthChanged,
            [this](const Event& evt) {
                const HealthChangedEvent& health = evt.data.healthChanged;
                
                if (health.newHealth <= 0) {
                    // Créer un événement de destruction
                    EntityDestroyedEvent destroyed;
                    destroyed.entity = health.entity;
                    destroyed.reason = "health_depleted";
                    destroyed.timestamp = SDL_GetTicks() / 1000.0f;
                    
                    Event destroyEvt = Event::CreateEntityDestroyed(destroyed);
                    World::Get().GetEventQueueSystem()->PostEvent(
                        ECSEventType::EntityDestroyed, destroyEvt);
                }
            });
    }
    
private:
    void ApplyDamageIfNeeded(EntityID entityA, EntityID entityB)
    {
        // Logique de dommages...
        if (World::Get().HasComponent<Health_data>(entityA)) {
            Health_data& health = World::Get().GetComponent<Health_data>(entityA);
            int oldHealth = health.currentHealth;
            health.currentHealth -= 10;
            
            // Poster un événement de changement de santé
            HealthChangedEvent healthEvt;
            healthEvt.entity = entityA;
            healthEvt.oldHealth = oldHealth;
            healthEvt.newHealth = health.currentHealth;
            healthEvt.delta = -10;
            healthEvt.instigator = entityB;
            healthEvt.timestamp = SDL_GetTicks() / 1000.0f;
            
            Event evt = Event::CreateHealthChanged(healthEvt);
            World::Get().GetEventQueueSystem()->PostEvent(
                ECSEventType::HealthChanged, evt);
        }
    }
};
```

### Exemple 3: Système de déclenchement (Trigger)

```cpp
class TriggerSystem : public ECS_System
{
public:
    TriggerSystem() {
        requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
        requiredSignature.set(GetComponentTypeID_Static<TriggerZone_data>(), true);
    }
    
    void Process() override
    {
        EventQueueSystem* eventQueue = World::Get().GetEventQueueSystem();
        
        for (EntityID trigger : m_entities) {
            TriggerZone_data& zone = World::Get().GetComponent<TriggerZone_data>(trigger);
            Position_data& triggerPos = World::Get().GetComponent<Position_data>(trigger);
            
            // Vérifier les entités dans la zone
            for (EntityID other : World::Get().m_entitySignatures) {
                if (other == trigger) continue;
                
                if (World::Get().HasComponent<Position_data>(other)) {
                    Position_data& otherPos = World::Get().GetComponent<Position_data>(other);
                    float distance = CalculateDistance(triggerPos.position, otherPos.position);
                    
                    bool wasTriggered = zone.triggered;
                    bool isTriggered = (distance <= zone.radius);
                    
                    // Événement d'entrée dans la zone
                    if (!wasTriggered && isTriggered) {
                        TriggerEvent triggerEvt;
                        triggerEvt.trigger = trigger;
                        triggerEvt.other = other;
                        triggerEvt.entered = true;
                        triggerEvt.timestamp = SDL_GetTicks() / 1000.0f;
                        
                        Event evt = Event::CreateTrigger(triggerEvt);
                        eventQueue->PostEvent(ECSEventType::TriggerEntered, evt);
                        
                        zone.triggered = true;
                    }
                    // Événement de sortie de la zone
                    else if (wasTriggered && !isTriggered) {
                        TriggerEvent triggerEvt;
                        triggerEvt.trigger = trigger;
                        triggerEvt.other = other;
                        triggerEvt.entered = false;
                        triggerEvt.timestamp = SDL_GetTicks() / 1000.0f;
                        
                        Event evt = Event::CreateTrigger(triggerEvt);
                        eventQueue->PostEvent(ECSEventType::TriggerExited, evt);
                        
                        zone.triggered = false;
                    }
                }
            }
        }
    }
};
```

## Avantages du système

### 1. Découplage
Les systèmes n'ont pas besoin de se connaître. Les événements servent d'interface.

### 2. Typage fort
Chaque événement est une structure typée, évitant les erreurs de cast et facilitant l'auto-complétion IDE.

### 3. Testabilité
```cpp
// Test unitaire simple
TEST(EventQueueTest, PostAndConsume) {
    EventQueueSystem eventQueue;
    
    InputButtonEvent btnEvt;
    btnEvt.buttonId = 0;
    btnEvt.pressed = true;
    
    Event evt = Event::CreateInputButton(btnEvt);
    eventQueue.PostEvent(ECSEventType::InputButton, evt);
    
    bool received = false;
    eventQueue.ConsumeEvents(ECSEventType::InputButton,
        [&](const Event& e) {
            received = true;
            ASSERT_EQ(e.data.inputButton.buttonId, 0);
        });
    
    ASSERT_TRUE(received);
}
```

### 4. Replay et debug
Les événements peuvent être enregistrés et rejoués pour reproduire des bugs:

```cpp
// Enregistrement
std::vector<Event> recordedEvents;
eventQueue->PeekEvents(ECSEventType::InputButton,
    [&](const Event& evt) {
        recordedEvents.push_back(evt);
    });

// Replay
for (const Event& evt : recordedEvents) {
    eventQueue->PostEvent(ECSEventType::InputButton, evt);
}
```

### 5. Moins d'état global
Les événements sont stockés dans des queues locales ou dans le système EventQueue, pas dans des singletons globaux.

## Bonnes pratiques

1. **Toujours vérifier le retour de PushEvent/PostEvent** - La queue peut être pleine
2. **Consommer les événements chaque frame** - Éviter l'accumulation
3. **Utiliser des timestamps** - Permet le tri et le replay temporel
4. **Documenter les producteurs/consommateurs** - Clarifier le flux d'événements
5. **Préférer PeekEvents pour le logging** - Ne pas perturber le traitement normal

## Migration depuis l'ancien système

### Avant (EventManager générique)
```cpp
Message msg;
msg.msg_type = EventType::Olympe_EventType_Object_CollideEvent;
msg.deviceId = entityA;
msg.controlId = entityB;
EventManager::Get().AddMessage(msg);
```

### Après (EventQueue typé)
```cpp
CollisionEvent collision;
collision.entityA = entityA;
collision.entityB = entityB;
collision.started = true;

Event evt = Event::CreateCollision(collision);
World::Get().GetEventQueueSystem()->PostEvent(
    ECSEventType::CollisionStarted, evt);
```

## Performance

- **Ring buffer** - Allocation fixe, pas de malloc/free pendant le jeu
- **Cache-friendly** - Accès séquentiel aux événements
- **O(1)** - Push et Pop en temps constant
- **Capacité** - 64 événements par queue (configurable via QUEUE_CAPACITY)

## Limitations

1. **Capacité fixe** - 64 événements maximum par queue (évite overflow en checkant le retour)
2. **Pas de priorités** - FIFO strict (utiliser plusieurs queues si nécessaire)
3. **Pas de filtrage** - Tous les consommateurs reçoivent tous les événements d'un type

## Extension

Pour ajouter un nouveau type d'événement:

1. Ajouter l'enum dans `ECSEventType`
2. Définir la structure d'événement dans `ECS_Events.h`
3. Ajouter le membre dans l'union `EventData`
4. Ajouter une méthode statique `Event::Create...()` 
5. Documenter l'utilisation dans ce guide

## Références

- `Source/ECS_Events.h` - Définitions des événements
- `Source/ECS_Components.h` - Composant EventQueue_data
- `Source/ECS_Systems.h` - Système EventQueueSystem
- `Source/ECS_Systems.cpp` - Implémentation du système

---

**Version**: 1.0  
**Date**: Décembre 2025  
**Auteur**: Nicolas Chereau  
*Olympe Engine V2 - 2025*
