# 📋 Résumé Exécutif - Variables Globales Blackboard

## ❓ Question Posée

**Identifier et lister les variables blackboard globales accessibles au runtime par les AI des entités. Ces variables globales sont-elles interfacées avec les Visual Graphes ATS ?**

---

## ✅ Réponse Courte

**OUI** — Les variables globales **SONT** accessibles et **SONT** complètement interfacées avec Visual Graphs ATS.

### Architecture Simple:

```
Visual Script (GetBBValue node with "global:" prefix)
         ↓
LocalBlackboard::GetValueScoped("global:VarName")
         ↓
GlobalBlackboard::Get().GetVar("VarName")
         ↓
Process-wide singleton storing all globals
```

---

## 🔍 Variables Globales Accessibles

### Comment identifier une variable globale?

Dans le template blackboard du blueprint:

```cpp
struct BlackboardEntry {
    std::string Key;      // "GamePhase", "WaveNumber", etc.
    VariableType Type;    // Bool, Int, Float, String, Vector, EntityID
    TaskValue Default;    // Default value
    bool IsGlobal;        // ✅ If TRUE → GLOBAL variable
};
```

### Où les définir?

1. **Blueprint Editor UI:**
   ```
   Properties Panel → Local Blackboard
   [+] Add key
   Type: (Float, Int, String, etc.)
   [Global checkbox] ✓ (checked = global)
   ```

2. **Template JSON (v4 schema):**
   ```json
   {
     "blackboard": [
       {
         "key": "GamePhase",
         "type": "String",
         "isGlobal": true     ← This makes it GLOBAL
       }
     ]
   }
   ```

### Types Supportés pour les Globals:

| Type | Exemple | Utilisation |
|------|---------|-------------|
| **Bool** | `IsGamePaused = true` | Flags d'état |
| **Int** | `WaveCount = 3` | Compteurs |
| **Float** | `Difficulty = 1.5` | Multiplicateurs |
| **String** | `GamePhase = "Combat"` | État texte |
| **Vector** | `SpawnPoint = (x,y,z)` | Positions |
| **EntityID** | `BossID = 42` | Références |

---

## 🔗 Interface avec Visual Graphs ATS

### Les nodes supportant les globals:

| Node | Support | Exemple |
|------|---------|---------|
| **GetBBValue** | ✅ YES | `GetBBValue("global:GamePhase")` |
| **SetBBValue** | ✅ YES | `SetBBValue("global:WaveCount", 5)` |
| **Branch** | ✅ YES | Condition: `global:GamePhase == "Combat"` |
| **Switch** | ✅ YES | `Switch on global:GamePhase` |
| **While** | ✅ YES | Loop while `global:IsRunning` |

### Scope Prefix Convention:

```cpp
// Reading a global variable
value = bb.GetValueScoped("global:GamePhase");     // ← GLOBAL
value = bb.GetValueScoped("local:MyLocalVar");     // ← LOCAL
value = bb.GetValueScoped("MyLocalVar");           // ← LOCAL (default)

// Writing a global variable
bb.SetValueScoped("global:WaveCount", TaskValue(5));
bb.SetValueScoped("local:MyLocalVar", TaskValue(42));
```

---

## 🏗️ Architecture Complète

```
┌──────────────────────────────────┐
│  Template Definition (Design)    │
│  TaskGraphTemplate.Blackboard    │
│  + BlackboardEntry {             │
│      Key, Type,                  │
│      Default,                    │
│      IsGlobal = true ← GLOBAL   │
│    }                             │
└────────────┬─────────────────────┘
             │ Serialize to JSON v4 schema
             ↓
┌──────────────────────────────────┐
│  Blueprint File                  │
│  "Blueprint.ats"                 │
│  {                               │
│    "blackboard": [               │
│      {                           │
│        "key": "GamePhase",       │
│        "isGlobal": true          │
│      }                           │
│    ]                             │
│  }                               │
└────────────┬─────────────────────┘
             │ Load & Initialize
             ↓
┌──────────────────────────────────┐
│  Runtime Execution               │
│                                  │
│  LocalBlackboard:                │
│  - Holds LOCAL vars only         │
│  - References GLOBAL via proxy   │
│                                  │
│  GlobalBlackboard (Singleton):   │
│  - Process-wide storage          │
│  - Shared by ALL AIs/Entities    │
│  - Accessed via "global:" prefix │
└──────────────────────────────────┘
```

---

## 📌 Key Facts

### ✅ Characteristics Positives:

1. **Cross-Entity Communication:**
   - ALL AIs read/write same global values
   - No need for explicit entity messaging
   - Real-time data sharing

2. **Completely Integrated with Visual Graphs:**
   - UI dropdowns show global variables ◆
   - JSON schema includes IsGlobal flag
   - Nodes (GetBBValue, SetBBValue, Branch, Switch) fully support

3. **Type-Safe:**
   - TaskValue enforces type checking
   - Compile-time type errors prevented

4. **Persistent During Game:**
   - Survives between graph executions
   - Survives entity spawn/despawn
   - Lives entire process lifetime

### ⚠️ Limitations:

1. **NOT Thread-Safe:** 
   - Need external synchronization if multi-threaded
   - Use mutex/atomic if concurrent access needed

2. **No Auto-Persistence to Disk:**
   - SaveToJson/LoadFromJson available but optional
   - Must be called explicitly

3. **Manual Cleanup:**
   - `GlobalBlackboard::Get().Clear()` needed
   - No auto-cleanup on level change

---

## 🎯 Common Global Variables Pattern

```cpp
// Game State Management
"GamePhase"              (String)  = "Exploration", "Combat", "Boss", "GameOver"
"IsGamePaused"           (Bool)    = false
"CurrentLevel"           (String)  = "Forest_01"

// Wave System
"WaveNumber"             (Int)     = 1
"EnemiesRemaining"       (Int)     = 10
"WaveSpawnerActive"      (Bool)    = true

// Difficulty
"EnemyDifficultyMult"    (Float)   = 1.0
"PlayerHealthMult"       (Float)   = 1.0

// Coordination
"BossFightStarted"       (Bool)    = false
"BossCurrentHealth"      (Float)   = 100.0
"BossPhase"              (String)  = "Phase1"
"SquadLeaderID"          (EntityID)= 0

// Events
"TotalEnemiesDefeated"   (Int)     = 0
"PlayerDiedCount"        (Int)     = 0
"BossLastAttackTime"     (Float)   = 0.0
```

---

## 📖 Files Involved

### Key Implementation Files:

| File | Purpose |
|------|---------|
| `GlobalBlackboard.h/cpp` | Singleton global storage (process-wide) |
| `LocalBlackboard.h/cpp` | Per-graph instance blackboard + proxy to global |
| `BBVariableRegistry.h/cpp` | Editor UI dropdown support |
| `TaskGraphTemplate.h` | Template definition with IsGlobal flag |
| `VisualScriptEditorPanel.cpp` | UI rendering with global indicator ◆ |

---

## 🔄 Complete Runtime Flow

```
1. User creates a blueprint with:
   Blackboard: GamePhase (String, isGlobal=true)

2. Blueprint saved as JSON v4:
   "blackboard": [{"key": "GamePhase", "isGlobal": true}]

3. At runtime, graph executes:
   GetBBValue("global:GamePhase")
   ↓
   LocalBlackboard::GetValueScoped("global:GamePhase")
   ↓
   Strips "global:" prefix
   ↓
   GlobalBlackboard::Get().GetVar("GamePhase")
   ↓
   Returns value from process-wide singleton

4. ALL entities see the SAME value:
   Entity 1 ─┐
   Entity 2  ├─→ GlobalBlackboard.GetVar("GamePhase")
   Entity 3 ─┘

   All get: "Combat" (or whatever was last set)
```

---

## 📊 Compatibility Matrix

| Feature | Status | Notes |
|---------|--------|-------|
| Define global vars in UI | ✅ YES | Via checkbox "Global" |
| Serialize to JSON v4 | ✅ YES | `"isGlobal": true` field |
| Access in Visual Scripts | ✅ YES | `"global:"` prefix |
| GetBBValue node support | ✅ YES | Full support |
| SetBBValue node support | ✅ YES | Full support |
| Branch conditions | ✅ YES | Can use `"global:VarName"` |
| Switch dispatch | ✅ YES | Can switch on `"global:VarName"` |
| Type safety | ✅ YES | TaskValue enforces |
| Cross-entity access | ✅ YES | Designed for this |
| Process lifetime | ✅ YES | Singleton pattern |
| Thread safety | ❌ NO | Needs external sync |
| Auto disk persistence | ❌ NO | Manual SaveToJson needed |

---

## ✅ Conclusion

**YES** — Global blackboard variables in Olympe Engine:

1. ✅ **ARE accessible** to all entity AIs at runtime
2. ✅ **ARE fully interfaced** with Visual Graphs ATS
3. ✅ **Support all node types** (GetBBValue, SetBBValue, Branch, Switch)
4. ✅ **Enable cross-entity coordination** without explicit messaging
5. ✅ **Are type-safe** and properly serialized in JSON v4 schema

**The implementation is complete and production-ready** for game state management and multi-entity coordination in the Olympe Engine's AI system.

---

### 📚 Additional Resources:

- **Detailed Architecture:** `GLOBAL_BLACKBOARD_ARCHITECTURE.md`
- **Use Cases:** `GLOBAL_BLACKBOARD_USECASES.md`
- **Source Code Locations:**
  - `Source/NodeGraphCore/GlobalBlackboard.h` (Singleton)
  - `Source/TaskSystem/LocalBlackboard.h` (Proxy + Scope handling)
  - `Source/BlueprintEditor/BBVariableRegistry.h` (UI support)

---

**Généré:** 2026-03-14 | Olympe Engine  
**Classification:** AI System Architecture  
**Phase:** 24 - Visual Script Complete
