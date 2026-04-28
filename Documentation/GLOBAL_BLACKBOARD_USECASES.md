# Cas d'Usage - Variables Globales dans Olympe Engine

**Variables Globales Accessibles aux Entity AIs**

---

## 📚 Index des Cas d'Usage

1. [Gestion d'État Global du Jeu](#gestion-détat-global-du-jeu)
2. [Coordination entre Entities](#coordination-entre-entities)
3. [Difficultéé et Progression](#difficultée-et-progression)
4. [Triggers et Événements Globaux](#triggers-et-événements-globaux)
5. [Synchronisation Multi-Entités](#synchronisation-multi-entités)

---

## 1. Gestion d'État Global du Jeu

### Cas: Game Phase / Level State

**Variables globales typiques:**
```
GamePhase (String, global)
  - Values: "Exploration" | "Combat" | "Boss" | "Cinematic" | "GameOver"
  - Accessible by: Tous les AIs (player allies, enemies, NPCs)
  - Utilisation: Décider du comportement selon la phase de jeu
```

**Blueprint Visual Script - AI Enemy:**

```json
{
  "name": "Enemy_CombatAI",
  "blackboard": [
    {
      "key": "GamePhase",
      "type": "String",
      "value": "Exploration",
      "isGlobal": true
    },
    {
      "key": "CurrentTarget",
      "type": "EntityID",
      "value": "0",
      "isGlobal": false
    }
  ],
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint"
    },
    {
      "id": 2,
      "type": "Branch",
      "label": "Check Game Phase"
    },
    {
      "id": 3,
      "type": "GetBBValue",
      "bbKey": "global:GamePhase"
    },
    {
      "id": 4,
      "type": "AtomicTask",
      "taskType": "ExecuteCombatRoutine"
    }
  ],
  "conditions": [
    {
      "leftMode": "Pin",
      "leftPin": "GetBBValue_3.Value",
      "operator": "==",
      "rightMode": "Const",
      "rightConstValue": "Combat"
    }
  ],
  "execConnections": [
    { "fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In" },
    { "fromNode": 2, "fromPin": "Then", "toNode": 4, "toPin": "In" }
  ]
}
```

**Runtime Behavior:**
```cpp
// During AI execution:
1. GetBBValue node reads: globalBB.GetVar("GamePhase")
2. If == "Combat" → Execute combat routine
3. If == "Exploration" → Execute patrol routine
4. If == "GameOver" → Execute retreat/hide routine
```

---

## 2. Coordination entre Entities

### Cas: Enemy Squad Formation

**Variables globales:**
```
SquadLeaderID (EntityID, global)
  - Qui est le leader du groupe
  - Mis à jour par GetBBValue si le leader meurt

SquadFormation (String, global)
  - "Tight" | "Loose" | "Flanking" | "Retreating"
  - Détermine la tactique de tous les membres
```

**Blueprint: Squad Member AI**

```json
{
  "name": "SquadMember_CombatAI",
  "blackboard": [
    {
      "key": "SquadLeaderID",
      "type": "EntityID",
      "value": "0",
      "isGlobal": true
    },
    {
      "key": "SquadFormation",
      "type": "String",
      "value": "Tight",
      "isGlobal": true
    },
    {
      "key": "MyHealthPercent",
      "type": "Float",
      "value": "1.0",
      "isGlobal": false
    }
  ],
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint"
    },
    {
      "id": 2,
      "type": "Switch",
      "switchVariable": "global:SquadFormation"
    },
    {
      "id": 3,
      "type": "AtomicTask",
      "taskType": "FormTightFormation"
    },
    {
      "id": 4,
      "type": "AtomicTask",
      "taskType": "FormLooseFormation"
    }
  ],
  "switchCases": [
    { "value": "Tight", "pin": "Case_0", "label": "Tight Formation" },
    { "value": "Loose", "pin": "Case_1", "label": "Loose Formation" }
  ]
}
```

**Synchronisation:**

```
Squad Member 1            Squad Member 2          Squad Member 3
┌──────────────┐         ┌──────────────┐        ┌──────────────┐
│ GetBBValue   │         │ GetBBValue   │        │ GetBBValue   │
│ SquadForm    │         │ SquadForm    │        │ SquadForm    │
└──────┬───────┘         └──────┬───────┘        └──────┬───────┘
       │                        │                       │
       └────────────┬───────────┴───────────┬───────────┘
                    │                       │
                GlobalBlackboard::Get().GetVar("SquadFormation")
                    ↓
            Read same value: "Tight"
                    ↓
            All 3 members execute:
            FormTightFormation() in parallel
```

---

## 3. Difficultée et Progression

### Cas: Wave Counter et Difficulty Scaling

**Variables globales:**
```
WaveNumber (Int, global)
  - Numéro de la vague actuelle (1, 2, 3, ...)

EnemyDifficultyMultiplier (Float, global)
  - Escalade: 1.0 → 1.2 → 1.5 → 2.0
  - Appliqué à la santé/dégâts de tous les ennemis

TotalEnemiesDefeated (Int, global)
  - Compteur global pour la progression
```

**Blueprint: Enemy Spawner**

```json
{
  "name": "WaveManager",
  "blackboard": [
    {
      "key": "WaveNumber",
      "type": "Int",
      "value": "1",
      "isGlobal": true
    },
    {
      "key": "EnemyDifficultyMultiplier",
      "type": "Float",
      "value": "1.0",
      "isGlobal": true
    }
  ],
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint"
    },
    {
      "id": 2,
      "type": "GetBBValue",
      "bbKey": "global:WaveNumber"
    },
    {
      "id": 3,
      "type": "MathOp",
      "mathOp": "+"
    },
    {
      "id": 4,
      "type": "SetBBValue",
      "bbKey": "global:WaveNumber"
    },
    {
      "id": 5,
      "type": "AtomicTask",
      "taskType": "SpawnEnemyWave"
    }
  ],
  "dataConnections": [
    {
      "fromNode": 2,
      "fromPin": "Value",
      "toNode": 3,
      "toPin": "A"
    }
  ]
}
```

**Scaling Enemy Health:**

```cpp
// Dans AtomicTask "SpawnEnemy":
void SpawnEnemy(LocalBlackboard& bb) {
    // Récupère le multiplicateur de difficulté global
    TaskValue diffMult = bb.GetValueScoped("global:EnemyDifficultyMultiplier");
    float multiplier = diffMult.AsFloat();  // e.g., 1.5

    // Crée l'ennemi avec santé escaladée
    int baseHealth = 100;
    int scaledHealth = static_cast<int>(baseHealth * multiplier);  // 150

    Enemy* enemy = new Enemy(scaledHealth);
    SpawnAtLocation(enemy, GetNextSpawnPoint());
}
```

---

## 4. Triggers et Événements Globaux

### Cas: Boss Encounter Trigger

**Variables globales:**
```
BossFightStarted (Bool, global)
  - false: Boss pas encore activé
  - true: Boss encounter en cours

BossCurrentHealth (Float, global)
  - Santé du boss (partagée avec tous les AIs allies)

BossPhase (String, global)
  - "Phase1" | "Phase2" | "Phase3"
  - Les AIs adaptent leur stratégie selon la phase
```

**Blueprint: Player AI Ally**

```json
{
  "name": "Ally_AIBehavior",
  "blackboard": [
    {
      "key": "BossFightStarted",
      "type": "Bool",
      "value": false,
      "isGlobal": true
    },
    {
      "key": "BossCurrentHealth",
      "type": "Float",
      "value": "0.0",
      "isGlobal": true
    }
  ],
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint"
    },
    {
      "id": 2,
      "type": "Branch",
      "label": "Is Boss Fight Active?"
    },
    {
      "id": 3,
      "type": "GetBBValue",
      "bbKey": "global:BossFightStarted"
    },
    {
      "id": 4,
      "type": "AtomicTask",
      "taskType": "ExecuteBossDefenseStrategy"
    },
    {
      "id": 5,
      "type": "AtomicTask",
      "taskType": "ExecutePatrolBehavior"
    }
  ],
  "conditions": [
    {
      "leftMode": "Pin",
      "leftPin": "3_Value",
      "operator": "==",
      "rightMode": "Const",
      "rightConstValue": "true"
    }
  ]
}
```

**Runtime Trigger Chain:**

```
Player encounters Boss
    ↓
Boss AI: SetBBValue("global:BossFightStarted", true)
    ↓
┌───────────────┬─────────────────┬──────────────┐
│               │                 │              │
↓               ↓                 ↓              ↓
Ally AI 1    Ally AI 2         Ally AI 3     Player AI
GetBBValue   GetBBValue       GetBBValue    GetBBValue
  "Boss*"      "Boss*"          "Boss*"       "Boss*"
    ↓           ↓                ↓             ↓
AllExecute:
ExecuteBossDefenseStrategy()

→ All allies coordinate defense
→ AI allies know boss health in real-time
→ Change phase strategy when boss reaches 50%
```

---

## 5. Synchronisation Multi-Entités

### Cas: Horde Attack Coordination

**Variables globales:**
```
HordeTriggerActive (Bool, global)
IsHordeConverging (Bool, global)
HordeTargetLocation (Vector, global)
EnemiesInHorde (Int, global)
```

**Sequence d'événements:**

```json
{
  "name": "HordeEnemy_AI",
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint"
    },
    {
      "id": 2,
      "type": "Branch",
      "label": "Should Join Horde?"
    },
    {
      "id": 3,
      "type": "GetBBValue",
      "bbKey": "global:HordeTriggerActive"
    },
    {
      "id": 4,
      "type": "GetBBValue",
      "bbKey": "global:HordeTargetLocation"
    },
    {
      "id": 5,
      "type": "SetBBValue",
      "bbKey": "global:EnemiesInHorde"
    },
    {
      "id": 6,
      "type": "AtomicTask",
      "taskType": "MoveToHordeTargetLocation"
    }
  ]
}
```

**Execution Timeline:**

```
Time: 0.0s
├─ Enemy 1 detects player
└─ Enemy 1: SetBBValue("global:HordeTriggerActive", true)

Time: 0.1s
├─ Enemy 2 sees global:HordeTriggerActive = true
├─ Enemy 2: Join horde
└─ Enemy 2: Increment global:EnemiesInHorde

Time: 0.2s
├─ Enemy 3, 4, 5 see HordeTriggerActive = true
├─ All get: global:HordeTargetLocation
└─ All move toward same target simultaneously

Result: Coordinated attack from multiple entities
        without explicit communication between them
        (only via global blackboard)
```

---

## 📊 Tableau de Comparaison: Local vs Global

| Aspect | Local Blackboard | Global Blackboard |
|--------|------------------|-------------------|
| **Scope** | Per-graph instance | Process-wide |
| **Sharing** | One graph only | All graphs + AIs |
| **Persistence** | Graph execution lifetime | Process lifetime |
| **Access Prefix** | `"local:"` or none | `"global:"` |
| **Use Case** | Temporary AI state | Game state / Coordination |
| **Thread-Safe** | Per-instance (safe) | ⚠️ NOT thread-safe |
| **Example Variables** | currentTarget, health | GamePhase, WaveNumber |
| **Init Source** | TaskGraphTemplate.Blackboard | Graph template (IsGlobal=true) |

---

## 🔧 Pattern: Global Variable Initialization

### Pattern 1: Boot-time Initialization

```cpp
// Au démarrage du jeu
void InitializeGameGlobals() {
    GlobalBlackboard& gb = GlobalBlackboard::Get();

    // Set default game state
    gb.SetVar("GamePhase", TaskValue(std::string("MainMenu")));
    gb.SetVar("WaveNumber", TaskValue(0));
    gb.SetVar("TotalEnemiesDefeated", TaskValue(0));
    gb.SetVar("GameDifficulty", TaskValue(1.0f));
    gb.SetVar("IsGamePaused", TaskValue(false));
}
```

### Pattern 2: Event-Triggered Update

```cpp
// Quand un événement se produit
void OnLevelStarted(Level* level) {
    GlobalBlackboard& gb = GlobalBlackboard::Get();

    gb.SetVar("CurrentLevelName", TaskValue(level->GetName()));
    gb.SetVar("GamePhase", TaskValue(std::string("Exploration")));
    gb.SetVar("BossFightStarted", TaskValue(false));
}
```

### Pattern 3: Querying Global State from AI

```cpp
// Dans la logique AI
void AIBrain::Update() {
    TaskValue phase = GlobalBlackboard::Get().GetVar("GamePhase");

    if (phase.AsString() == "Combat") {
        ExecuteCombatLogic();
    } else if (phase.AsString() == "Exploration") {
        ExecutePatrolLogic();
    }
}
```

---

## ⚠️ Bonnes Pratiques

### ✅ À FAIRE:

1. **Initialiser les variables globales au boot:**
   ```cpp
   // GOOD: Set defaults in InitializeGameGlobals()
   gb.SetVar("GamePhase", TaskValue("MainMenu"));
   ```

2. **Utiliser des noms descriptifs:**
   ```cpp
   // GOOD: Clear intent
   "GamePhase", "WaveNumber", "BossCurrentHealth"

   // BAD: Cryptic names
   "gp", "w", "bh"
   ```

3. **Regrouper les globals par système:**
   ```cpp
   // Game state
   "GamePhase", "IsGamePaused", "CurrentLevel"

   // Wave/Difficulty
   "WaveNumber", "EnemyDifficultyMultiplier"

   // Boss/Encounters
   "BossFightStarted", "BossCurrentHealth", "BossPhase"
   ```

4. **Documenter les variables:**
   ```cpp
   // GOOD: Comment explains usage
   // GamePhase (String): "MainMenu" | "Exploration" | "Combat" | "GameOver"
   gb.SetVar("GamePhase", TaskValue("Combat"));
   ```

### ❌ À NE PAS FAIRE:

1. **Pas de synchronisation cross-thread:**
   ```cpp
   // BAD: GlobalBlackboard not thread-safe
   // If multiple threads access simultaneously → data race
   ```

2. **Pas de stockage de pointeurs/références:**
   ```cpp
   // BAD: Can't store Entity* in TaskValue
   // OK: Store EntityID instead
   gb.SetVar("BossID", TaskValue(boss->GetID()));
   ```

3. **Pas de sur-utilisation:**
   ```cpp
   // GOOD: Only truly global state
   gb.SetVar("GamePhase", ...);

   // BAD: Local AI state in global (defeats purpose)
   gb.SetVar("MyUniqueEnemyLocalState", ...);  // ← Should be local
   ```

---

## 📈 Performance Considerations

### Lookup Performance:
- **Local Blackboard:** O(1) unordered_map lookup
- **Global Blackboard:** O(1) unordered_map lookup
- **Difference:** Negligible

### Memory:
- **Global variables persist** for entire process lifetime
- **Clear unused globals** if running long sessions
- **No automatic cleanup** - manual Clear() needed

### Best Practices:
```cpp
// Periodic cleanup of unused globals
void CleanupGameGlobals() {
    GlobalBlackboard& gb = GlobalBlackboard::Get();

    // Only keep essential state
    gb.Clear();  // Remove all

    // Re-initialize core systems
    InitializeGameGlobals();
}
```

---

## 🎯 Résumé

**Les variables globales dans Olympe Engine:**

✅ Partagées entre TOUS les AIs
✅ Accessibles via scope prefix `"global:"` dans Visual Graphs
✅ Initialisées depuis le template blackboard (IsGlobal=true)
✅ Centralisées via GlobalBlackboard singleton
✅ Type-safe grâce à TaskValue
✅ Parfait pour: game state, coordination, events
❌ Pas thread-safe nativement
❌ Pas persistées sur disque par défaut

**Patterns de coordination habituels:**
- Squad formation / Horde attacks
- Game phase management
- Wave/Difficulty scaling
- Boss encounter coordination
- Global event triggers

---

**Généré:** 2026-03-14 | Olympe Engine | Global Blackboard Use Cases
