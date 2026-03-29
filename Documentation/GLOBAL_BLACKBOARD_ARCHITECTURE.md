# Architecture du Système Global Blackboard - Olympe Engine
## Variables Globales Accessibles aux IAs Entities à Runtime

**Date:** 2026-03-14  
**Version:** 1.0  
**Status:** Phase 24 - Visual Script Complete

---

## 📋 Table des Matières

1. [Vue d'ensemble de l'architecture](#vue-densemble-de-larchitecture)
2. [Variables globales accessibles](#variables-globales-accessibles)
3. [Interface entre Global Blackboard et Visual Graphs ATS](#interface-entre-global-blackboard-et-visual-graphs-ats)
4. [Chaîne d'accès complète (runtime)](#chaîne-daccès-complète-runtime)
5. [Intégration dans les Visual Graphs](#intégration-dans-les-visual-graphs)
6. [Exemples de code](#exemples-de-code)

---

## Vue d'ensemble de l'architecture

Le système de blackboard du moteur Olympe utilise une **architecture à trois niveaux**:

### Architecture à 3 niveaux

```
┌─────────────────────────────────────────────────────────────┐
│ GLOBAL BLACKBOARD (Singleton Process-Wide)                  │
│ - Partagé entre TOUS les AIs et tous les graphes            │
│ - TaskValue map: <string key, TaskValue>                    │
│ - Scope prefix: "global:"                                   │
│ - Classes: GlobalBlackboard (singleton Meyers pattern)      │
└─────────────────────────────────────────────────────────────┘
                         ▲
                         │ GetValueScoped / SetValueScoped
                         │ avec prefix "global:"
                         ▼
┌─────────────────────────────────────────────────────────────┐
│ LOCAL BLACKBOARD PER GRAPH INSTANCE (Runtime)              │
│ - LocalBlackboard (par TaskRunner / VS graph execution)    │
│ - Contains: local + référencé aux globals                  │
│ - Scope prefix: "local:" (ou pas de prefix = local)        │
│ - Inicalisé depuis TaskGraphTemplate.Blackboard            │
└─────────────────────────────────────────────────────────────┘
                         ▲
                         │ Initialize / InitializeFromEntries
                         │ (skip IsGlobal=true entries)
                         ▼
┌─────────────────────────────────────────────────────────────┐
│ TEMPLATE BLACKBOARD (Blueprint Definition)                  │
│ - TaskGraphTemplate.Blackboard vector<BlackboardEntry>     │
│ - Chaque entry: {Key, Type, Default, IsGlobal flag}       │
│ - Stockage: Blueprint JSON v4 schema (embedded)            │
│ - Classes: BlackboardEntry struct                          │
└─────────────────────────────────────────────────────────────┘
```

---

## Variables globales accessibles

### 1. **Définition et Accès**

Les variables globales sont définies dans le **Template Blackboard** avec le flag `IsGlobal = true`:

```cpp
struct BlackboardEntry {
    std::string Key;              // Clé variable (ex: "GamePhase", "WaveCount")
    VariableType Type;            // Type: Bool, Int, Float, String, Vector, EntityID
    TaskValue Default;            // Valeur par défaut initiale
    bool IsGlobal;                // true = global scope ✓
    // ...
};
```

### 2. **Types supportés pour les variables globales**

| Type | Réprésentation | Exemple | Utilisation typique |
|------|---|---|---|
| **Bool** | `VariableType::Bool` | `IsGamePaused = true` | Flags d'état jeu |
| **Int** | `VariableType::Int` | `WaveCount = 5` | Compteurs, indices |
| **Float** | `VariableType::Float` | `GameDifficulty = 1.5f` | Modificateurs, ratios |
| **String** | `VariableType::String` | `CurrentLevel = "Forest_01"` | Identifiants texte |
| **Vector** | `VariableType::Vector` | `PlayerSpawnPoint = (x,y,z)` | Positions monde |
| **EntityID** | `VariableType::EntityID` | `BossEntityID = 42` | Références entités |

### 3. **Accès au Runtime**

Les variables globales sont accédées via le **LocalBlackboard** qui agit comme proxy:

```cpp
// Dans l'implémentation d'un noeud Visual Script:

// ACCÈS GLOBAL (avec scope prefix)
TaskValue value = localBlackboard.GetValueScoped("global:GamePhase");
localBlackboard.SetValueScoped("global:WaveCount", TaskValue(5));

// ACCÈS LOCAL (sans prefix ou "local:" explicite)
TaskValue localVal = localBlackboard.GetValueScoped("MyLocalVar");
localBlackboard.SetValueScoped("local:MyLocalVar", TaskValue(42));
```

---

## Interface entre Global Blackboard et Visual Graphs ATS

### 1. **Architecture d'Interface**

```
Visual Script Node (ex: GetBBValue, SetBBValue, Branch condition)
         │
         ▼
TaskNodeDefinition.BBKey = "GamePhase"  [local ou global:prefix]
         │
         ▼ Runtime Execution
TaskRunner.m_localBlackboard.GetValueScoped("GamePhase")
         │
         ├─ Si "local:" → m_localBlackboard.GetValue()
         │
         └─ Si "global:" → GlobalBlackboard::Get().GetVar()
                              │
                              ▼ (Singleton acces)
                         Process-Wide Global Storage
```

### 2. **Hiérarchie d'Interface**

```cpp
// Niveau 1: Visual Script Editor
class VisualScriptEditorPanel {
    // BBVariableRegistry pour populating dropdowns
    BBVariableRegistry m_variableRegistry;
    // Affiche local + global variables dans UI
};

// Niveau 2: Blueprint Template
class TaskGraphTemplate {
    std::vector<BlackboardEntry> Blackboard;  // Mixte local + global
    std::vector<ConditionPreset> Presets;     // Embedded presets
};

// Niveau 3: Runtime Execution
class TaskRunner {
    LocalBlackboard m_localBlackboard;  // Initialized avec non-global entries
    // Accès globals via LocalBlackboard::GetValueScoped("global:...")
};

// Niveau 4: Global Storage (Singleton)
class GlobalBlackboard {
    static GlobalBlackboard& Get();
    TaskValue GetVar(const std::string& key);
    void SetVar(const std::string& key, const TaskValue& value);
};
```

### 3. **Cycle Complet: Editor → Runtime**

1. **Editor (Conception)**
   - User ajoute une variable au template blackboard
   - Flag `IsGlobal = true` → variable sera partagée
   - Serialize dans `Blueprint.ats` (JSON v4 schema)

2. **Template Load (Chargement)**
   - TaskGraphTemplate désérialisé du JSON
   - Blackboard vector chargé avec local + global entries

3. **Runtime Init (Initialisation)**
   ```cpp
   TaskRunner runner;
   runner.Initialize(template);
   // Appel interne:
   m_localBlackboard.InitializeFromEntries(template.Blackboard);
   // Skips: entry.IsGlobal == true
   // (globals pas copiées, accédées via singleton)
   ```

4. **Graph Execution (Exécution)**
   - Node GetBBValue appelle:
     ```cpp
     value = GetValueScoped("global:GamePhase");
     // Redirigé à GlobalBlackboard::Get().GetVar("GamePhase");
     ```

5. **Persistence (Sauvegarde)**
   - Global state survit entre graph executions
   - Peut être saved/loaded via JSON si nécessaire

---

## Chaîne d'accès complète (runtime)

### Path de lecture (GetValueScoped)

```
Entity AI executing VS Graph
         │
         ├─ Node: GetBBValue("global:CurrentLevel")
         │
         ├─ LocalBlackboard::GetValueScoped("global:CurrentLevel")
         │
         ├─ String starts with "global:" → strip prefix
         │
         ├─ Call: GlobalBlackboard::Get().GetVar("CurrentLevel")
         │
         ├─ Lookup in m_store map
         │
         └─ Return TaskValue or default TaskValue() if not found
            (stored in Entity AI brain for decision)
```

### Path d'écriture (SetValueScoped)

```
Entity AI executing VS Graph
         │
         ├─ Node: SetBBValue("global:WaveCount", TaskValue(3))
         │
         ├─ LocalBlackboard::SetValueScoped("global:WaveCount", value)
         │
         ├─ String starts with "global:" → strip prefix
         │
         ├─ Call: GlobalBlackboard::Get().SetVar("WaveCount", value)
         │
         ├─ Update m_store["WaveCount"] = value
         │
         ├─ Mark: m_dirty = true (changed)
         │
         └─ Accessible by ALL other AIs immediately
            (next frame or same frame depending on exec order)
```

---

## Intégration dans les Visual Graphs

### 1. **Nodes impliquées**

| Node Type | Role | Accès Scope |
|-----------|------|-------------|
| **GetBBValue** | Lire variable (local ou global) | GetValueScoped() |
| **SetBBValue** | Écrire variable (local ou global) | SetValueScoped() |
| **Branch** | Condition sur variable | GetValueScoped() dans condition eval |
| **Switch** | Multi-path sur variable | GetValueScoped() pour dispatch |
| **While** | Boucle sur condition variable | GetValueScoped() chaque itération |

### 2. **Sérialization JSON (v4 schema)**

```json
{
  "schema_version": 4,
  "name": "AI_Combat",
  "blackboard": [
    {
      "key": "GamePhase",
      "type": "String",
      "value": "Combat",
      "isGlobal": true
    },
    {
      "key": "LocalTargetID",
      "type": "EntityID",
      "value": "0",
      "isGlobal": false
    }
  ],
  "nodes": [
    {
      "id": 101,
      "type": "GetBBValue",
      "bbKey": "global:GamePhase"
    }
  ]
}
```

### 3. **Rendu UI (Editor)**

Les variables globales sont distinguées dans la UI:

```
┌─ Node Properties Panel
│  ├─ [GetBBValue]
│  │  └─ Select variable:
│  │     ├─ health (Float, local)           ← Local scope
│  │     ├─ targetID (EntityID, local)      ← Local scope
│  │     ├─ GamePhase (String, global) ◆    ← GLOBAL (marqué différent)
│  │     └─ WaveCount (Int, global) ◆       ← GLOBAL
```

---

## Exemples de code

### Exemple 1: Lire une variable globale

```cpp
// Dans un noeud Visual Script GetBBValue
class GetBBValueNode {
    void Execute(LocalBlackboard& bb, TaskNodeContext& ctx) {
        std::string key = GetNodeParameter<std::string>("bbKey");

        // Retrieve value (auto-handles "global:" prefix)
        TaskValue value = bb.GetValueScoped(key);

        // Output pin receives the value
        SetOutputPin("Value", value);
    }
};

// Usage: 
// - bbKey = "global:GamePhase" → reads from GlobalBlackboard
// - bbKey = "local:PlayerHealth" → reads from LocalBlackboard
// - bbKey = "EnemyCount" → reads from LocalBlackboard (default local)
```

### Exemple 2: Écrire une variable globale

```cpp
// Dans un noeud Visual Script SetBBValue
class SetBBValueNode {
    void Execute(LocalBlackboard& bb, TaskNodeContext& ctx) {
        std::string key = GetNodeParameter<std::string>("bbKey");
        TaskValue newValue = GetInputPin("Value");

        // Write value (auto-handles "global:" prefix)
        bb.SetValueScoped(key, newValue);

        // Execution flows out
        ExecOut();
    }
};

// Usage:
// - bbKey = "global:WaveCount", value = 5 → sets GlobalBlackboard
// - bbKey = "local:CurrentTarget", value = entity_id → sets LocalBlackboard
```

### Exemple 3: Condition sur variable globale

```cpp
// Dans un noeud Branch utilisant une condition preset
class BranchNode {
    void Execute(LocalBlackboard& bb, TaskNodeContext& ctx) {
        // Évalue des conditions définies dans le preset
        for (auto& condition : m_conditionRefs) {
            // Left operand peut être "global:GamePhase"
            TaskValue left = bb.GetValueScoped(condition.leftOperand);

            // Right operand peut être constant ou "global:DifficultyLevel"
            TaskValue right = bb.GetValueScoped(condition.rightOperand);

            // Evaluate: left op right
            bool result = EvaluateCondition(left, right, condition.op);

            if (result) {
                ExecPin("Then");
                return;
            }
        }
        ExecPin("Else");
    }
};
```

### Exemple 4: Direct access au Global Blackboard (sans Visual Graph)

```cpp
// Accès directe depuis l'AI entity
class AIBrain {
    void Update() {
        // Direct access to global storage
        TaskValue gamePhase = GlobalBlackboard::Get().GetVar("GamePhase");

        if (gamePhase.AsString() == "Combat") {
            // Execute combat behavior
        } else if (gamePhase.AsString() == "Exploration") {
            // Execute exploration behavior
        }

        // Update global state
        GlobalBlackboard::Get().SetVar("EnemyCount", TaskValue(current_enemies));
    }
};
```

---

## Characteristics Importantes

### 1. **Persistence du Global Blackboard**
- ✅ Survit entre exécutions de graphes
- ✅ Partagé entre TOUS les AIs/Entities
- ✅ Valide pendant toute la durée du processus
- ⚠️ Pas de persistence sur disque par défaut (opt-in via SaveToJson/LoadFromJson)

### 2. **Thread Safety**
- ⚠️ ATTENTION: GlobalBlackboard n'est PAS thread-safe
- Nécessite synchronisation externe si plusieurs threads accèdent
- À implémenter: mutex ou atomic operations si concurrent access

### 3. **Type Safety**
- ✅ TaskValue enforce type checking
- ✅ SetValue() valide type match
- ⚠️ GetValue() retourne default TaskValue() si not found (pas d'erreur)

### 4. **Variable Scope Resolution**
- `"global:VarName"` → GlobalBlackboard (singleton)
- `"local:VarName"` → LocalBlackboard (per-graph instance)
- `"VarName"` (no prefix) → LocalBlackboard (default local)

---

## Résumé de l'Interface

```
┌─────────────────────────────────────────────────────────────┐
│  VISUAL SCRIPT BLUEPRINT (JSON v4)                          │
│                                                             │
│  "blackboard": [                                            │
│    { "key": "GamePhase", "isGlobal": true },  ← GLOBAL    │
│    { "key": "LocalState", "isGlobal": false } ← LOCAL     │
│  ]                                                         │
│                                                             │
│  "nodes": [                                                 │
│    { "type": "GetBBValue", "bbKey": "global:GamePhase" }  │
│  ]                                                         │
└──────────────┬────────────────────────────────┬────────────┘
               │                                │
        ┌──────▼──────┐              ┌─────────▼────────┐
        │ TEMPLATE     │              │ TEMPLATE         │
        │ LOAD         │              │ RUNTIME INIT     │
        └──────┬───────┘              └────────┬─────────┘
               │                              │
        ┌──────▼────────────────────────────▼──────┐
        │    TaskGraphTemplate                     │
        │    + LocalBlackboard (exec instance)     │
        └──────┬──────────────────────────┬────────┘
               │                          │
               │ GetValueScoped("global:") │
               │         +                │ InitializeFromEntries()
               │ SetValueScoped("global:") │ [skips IsGlobal=true]
               │                          │
        ┌──────▼──────────────────────────▼──────┐
        │  LocalBlackboard::SetValueScoped()     │
        │  Strips "global:" prefix              │
        └──────┬──────────────────────────┬──────┘
               │                          │
               │ Redirect to              │ Direct local access
               │ GlobalBlackboard         │ to LocalBlackboard
               │                          │
        ┌──────▼──────┐           ┌──────▼──────┐
        │ GlobalBlack  │           │ LocalBlack  │
        │ board        │           │ board       │
        │ (Singleton)  │           │ (per-graph) │
        └─────────────┘           └────────────┘
               ▲
               │ GetVar("key") / SetVar("key")
               │ SHARED across ALL AIs/Entities
               │
        Process-wide global state
```

---

## Conclusion

✅ **Les variables globales SONT accessibles aux AIs entities à runtime via:**
1. Le flag `IsGlobal = true` dans le template blackboard
2. Le scope prefix `"global:"` lors de l'accès (GetValueScoped/SetValueScoped)
3. Le GlobalBlackboard singleton qui stocke les valeurs process-wide

✅ **Visual Graphs ATS interface complètement avec le système global:**
1. Les nodes GetBBValue/SetBBValue supportent both local et global scope
2. Les conditions Branch/While peuvent utiliser variables globales
3. La sérialization JSON v4 inclut l'information IsGlobal
4. L'UI Editor distingue et affiche les variables globales vs locales

✅ **Architecture est bien structurée:** Template → Runtime → Global Singleton

---

**Généré:** 2026-03-14 | Olympe Engine | Visual Script Phase 24
