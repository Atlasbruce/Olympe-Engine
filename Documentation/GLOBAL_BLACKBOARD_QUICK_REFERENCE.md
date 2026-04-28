# 🎯 Global Blackboard - Quick Reference Card

## At a Glance

```
Global Blackboard Variables in Olympe Engine

QUESTION: Are global blackboard variables accessible to Entity AIs at runtime 
          and interfaced with Visual Graphs ATS?

ANSWER:   ✅ YES - Complete integration

═══════════════════════════════════════════════════════════════════════════════
```

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ EDITOR TIME (Design Phase)                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. Create Blueprint Variable:                                              │
│     ┌────────────────────────────────────────┐                              │
│     │ Properties Panel                       │                              │
│     │                                        │                              │
│     │ Name: GamePhase                        │                              │
│     │ Type: String                           │                              │
│     │ Default: "Exploration"                 │                              │
│     │ ☑ Global        ← CHECK THIS!          │                              │
│     └────────────────────────────────────────┘                              │
│                                                                              │
│  2. Use in Blueprint:                                                       │
│     ┌────────────────────────────────────────┐                              │
│     │ GetBBValue("global:GamePhase")         │                              │
│     │ SetBBValue("global:WaveNumber", 5)     │                              │
│     │ Branch: "global:GamePhase" == "Combat" │                              │
│     └────────────────────────────────────────┘                              │
│                                                                              │
│  3. Save to Blueprint.ats:                                                  │
│     {                                                                       │
│       "blackboard": [                                                       │
│         {                                                                   │
│           "key": "GamePhase",                                               │
│           "type": "String",                                                 │
│           "isGlobal": true   ← Serialized!                                  │
│         }                                                                   │
│       ]                                                                     │
│     }                                                                       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ RUNTIME (Execution Phase)                                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Entity AI 1                Entity AI 2                Entity AI 3          │
│  ┌─────────────┐           ┌─────────────┐           ┌─────────────┐      │
│  │ GetBBValue  │           │ GetBBValue  │           │ SetBBValue  │      │
│  │ "global:    │           │ "global:    │           │ "global:    │      │
│  │ GamePhase"  │           │ GamePhase"  │           │ GamePhase"  │      │
│  └──────┬──────┘           └──────┬──────┘           └──────┬──────┘      │
│         │                         │                         │              │
│         └──────────┬──────────────┴─────────────┬──────────┘              │
│                    │                           │                          │
│                    ▼                           ▼                          │
│        LocalBlackboard::GetValueScoped("global:GamePhase")                 │
│                          │                                                 │
│                    ┌─────┴──────┐                                          │
│                    │             │                                        │
│                    ▼             ▼                                        │
│          Strip "global:" prefix → "GamePhase"                             │
│                    │                                                       │
│                    ▼                                                       │
│   GlobalBlackboard::Get().GetVar("GamePhase")                             │
│                    │                                                       │
│                    ▼                                                       │
│   ┌───────────────────────────────┐                                       │
│   │ GLOBAL BLACKBOARD SINGLETON   │                                       │
│   │ (Process-wide, shared)        │                                       │
│   │                               │                                       │
│   │ m_store = {                   │                                       │
│   │   "GamePhase": "Combat",      │◄──── ALL AIs read/write this         │
│   │   "WaveNumber": 5,            │                                       │
│   │   "BossHealth": 250.0,        │                                       │
│   │   ...                         │                                       │
│   │ }                             │                                       │
│   └───────────────────────────────┘                                       │
│                    ▲                                                       │
│                    │                                                       │
│         Return "Combat" to all requesting AIs                             │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick Lookup Table

### When to use GLOBAL:

```
✅ GLOBAL (isGlobal = true)
├─ Game phase / Level state
├─ Wave management
├─ Difficulty scaling
├─ Boss encounter state
├─ Cross-entity coordination
├─ Shared achievements
├─ Global event flags
└─ Anything needed by MULTIPLE AIs

❌ LOCAL (isGlobal = false)
├─ Current target entity
├─ Internal AI state
├─ Temporary calculations
├─ Per-entity statistics
├─ Behavior tree state
└─ Anything UNIQUE to one AI
```

---

## Access Pattern Cheat Sheet

```
╔═══════════════════════════════════════════════════════════════════════════╗
║ PATTERN                                  RESULT                           ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                           ║
║ GetBBValue("global:GamePhase")                                           ║
║   → LocalBlackboard.GetValueScoped("global:GamePhase")                  ║
║   → GlobalBlackboard.Get().GetVar("GamePhase")                          ║
║   → Read from GLOBAL SINGLETON ✅                                         ║
║                                                                           ║
║ GetBBValue("local:MyVar")                                                ║
║   → LocalBlackboard.GetValueScoped("local:MyVar")                       ║
║   → LocalBlackboard.GetValue("MyVar")                                    ║
║   → Read from THIS GRAPH'S LOCAL blackboard ✅                            ║
║                                                                           ║
║ GetBBValue("MyVar")  [no prefix]                                         ║
║   → LocalBlackboard.GetValueScoped("MyVar")                             ║
║   → LocalBlackboard.GetValue("MyVar")  [default to local] ✅             ║
║                                                                           ║
║ SetBBValue("global:WaveNumber", 5)                                       ║
║   → LocalBlackboard.SetValueScoped("global:WaveNumber", TaskValue(5))   ║
║   → GlobalBlackboard.Get().SetVar("WaveNumber", TaskValue(5))           ║
║   → Write to GLOBAL SINGLETON ✅                                         ║
║   → ALL OTHER AIs immediately see new value!                             ║
║                                                                           ║
╚═══════════════════════════════════════════════════════════════════════════╝
```

---

## Implementation Checklist

```
☑️ Global Blackboard System
   ├─ ✅ GlobalBlackboard singleton (Meyers pattern)
   ├─ ✅ Process-wide TaskValue storage
   ├─ ✅ GetVar / SetVar methods
   └─ ✅ IsDirty / MarkDirty tracking

☑️ Integration with Visual Graphs
   ├─ ✅ LocalBlackboard::GetValueScoped() proxy
   ├─ ✅ LocalBlackboard::SetValueScoped() proxy
   ├─ ✅ Scope prefix handling ("global:" vs "local:")
   └─ ✅ InitializeFromEntries() skip IsGlobal=true

☑️ Blueprint Schema
   ├─ ✅ BlackboardEntry.IsGlobal field
   ├─ ✅ JSON v4 serialization of "isGlobal"
   ├─ ✅ TaskGraphTemplate.Blackboard vector
   └─ ✅ Editor UI checkbox for "Global"

☑️ Node Support
   ├─ ✅ GetBBValue accepts "global:VarName"
   ├─ ✅ SetBBValue accepts "global:VarName"
   ├─ ✅ Branch conditions use GetValueScoped()
   ├─ ✅ Switch dispatch uses GetValueScoped()
   └─ ✅ While loop conditions use GetValueScoped()

☑️ UI Integration
   ├─ ✅ BBVariableRegistry shows global indicator ◆
   ├─ ✅ Dropdown distinguishes global vs local
   ├─ ✅ Editor displays format: "name (Type, scope)"
   └─ ✅ Global variables sorted and displayed

☑️ Type System
   ├─ ✅ Bool support
   ├─ ✅ Int support
   ├─ ✅ Float support
   ├─ ✅ String support
   ├─ ✅ Vector support
   ├─ ✅ EntityID support
   └─ ✅ TaskValue enforces type matching
```

---

## Common Variables Reference

```
GAME STATE
├─ GamePhase (String)
│  └─ Values: "MainMenu" → "Exploration" → "Combat" → "Boss" → "GameOver"
├─ IsGamePaused (Bool)
├─ CurrentLevel (String)
└─ GameTime (Float)

WAVE SYSTEM
├─ WaveNumber (Int)
├─ EnemiesRemaining (Int)
├─ WaveSpawnerActive (Bool)
└─ NextSpawnTime (Float)

DIFFICULTY
├─ DifficultyLevel (Int)
├─ EnemyHealthMultiplier (Float)
├─ EnemyDamageMultiplier (Float)
└─ PlayerHealthMultiplier (Float)

BOSS ENCOUNTER
├─ BossFightStarted (Bool)
├─ BossCurrentHealth (Float)
├─ BossMaxHealth (Float)
├─ BossPhase (String)
│  └─ Values: "Phase1" → "Phase2" → "Phase3"
└─ BossLastAttackTime (Float)

COORDINATION
├─ SquadLeaderID (EntityID)
├─ SquadFormation (String)
│  └─ Values: "Tight" | "Loose" | "Flanking" | "Retreating"
├─ HordeTriggerActive (Bool)
├─ HordeTargetLocation (Vector)
└─ EnemiesInHorde (Int)

TRACKING
├─ TotalEnemiesDefeated (Int)
├─ PlayerDeathCount (Int)
├─ AchievementsUnlocked (Int)
└─ PlaySessionTime (Float)
```

---

## Scope Resolution Priority

```
When accessing a blackboard variable:

1. Input string: "MyVariable"
2. Scope check:

   If starts with "global:"
   └─ → Strip prefix → GlobalBlackboard.GetVar()

   If starts with "local:"
   └─ → Strip prefix → LocalBlackboard.GetValue()

   If no prefix (default)
   └─ → LocalBlackboard.GetValue() [treats as local]

EXAMPLE:
"global:GamePhase"    → GlobalBlackboard.GetVar("GamePhase")
"local:MyVar"         → LocalBlackboard.GetValue("MyVar")
"PlayerHealth"        → LocalBlackboard.GetValue("PlayerHealth")
```

---

## File Locations

```
Key Implementation Files:

Source/NodeGraphCore/
├─ GlobalBlackboard.h          ← Singleton definition
└─ GlobalBlackboard.cpp        ← Singleton implementation

Source/TaskSystem/
├─ LocalBlackboard.h           ← Proxy to global
└─ LocalBlackboard.cpp         ← Scope handling

Source/BlueprintEditor/
├─ BBVariableRegistry.h        ← UI support
└─ BBVariableRegistry.cpp      ← Registry implementation

Source/TaskSystem/
└─ TaskGraphTemplate.h         ← BlackboardEntry struct
```

---

## Troubleshooting

```
PROBLEM: Global variable not updating across entities

CHECK:
  1. Is IsGlobal = true in the template blackboard?
  2. Are you using "global:" prefix in GetBBValue/SetBBValue?
  3. Is the variable being initialized at game start?
  4. Are there multiple graph instances and only one updating?

SOLUTION:
  1. Edit blueprint blackboard, set ☑ Global
  2. Use GetBBValue("global:VarName"), SetBBValue("global:VarName", value)
  3. Call GlobalBlackboard::Get().SetVar() at game initialization
  4. Verify SetBBValue is connected in graph execution path

═════════════════════════════════════════════════════════════════════════════════

PROBLEM: Variable value defaults to None/empty

CHECK:
  1. Did you initialize the global before reading?
  2. Is the variable name spelled correctly?
  3. Is the type matching between read and write?

SOLUTION:
  1. GlobalBlackboard::Get().SetVar("VarName", TaskValue(defaultValue))
  2. Double-check spelling in GetBBValue node
  3. Ensure TaskValue type matches (Bool, Int, Float, String, Vector, EntityID)

═════════════════════════════════════════════════════════════════════════════════

PROBLEM: Multiple threads accessing global causing crashes

CHECK:
  1. GlobalBlackboard is NOT thread-safe
  2. Multiple threads accessing simultaneously?

SOLUTION:
  1. Add mutex around GlobalBlackboard access
  2. Synchronize thread access to GetVar/SetVar calls
  3. Consider per-thread local blackboards if possible
```

---

## Key Takeaways

```
✅ Global variables EXIST and are FULLY WORKING

✅ Complete integration with Visual Graphs
   - Templates define globals with IsGlobal flag
   - Nodes (GetBBValue, SetBBValue, Branch, Switch) support "global:" prefix
   - UI displays global variables with special indicator ◆

✅ Singleton pattern ensures single global storage
   - GlobalBlackboard::Get() from anywhere
   - All AIs read/write same values
   - Process lifetime persistence

✅ Type-safe via TaskValue system
   - Bool, Int, Float, String, Vector, EntityID supported
   - Type checking enforced

✅ Perfect for:
   - Game state management (phase, paused, level)
   - Multi-entity coordination (squads, hordes)
   - Wave/difficulty scaling
   - Boss encounter management
   - Cross-entity communication without explicit messaging

⚠️  Limitations:
   - NOT thread-safe (need external synchronization)
   - NOT auto-persisted to disk
   - NOT automatically cleared (manual cleanup needed)
```

---

**Last Updated:** 2026-03-14  
**Status:** ✅ Production Ready  
**Olympe Engine - Visual Script Phase 24**
