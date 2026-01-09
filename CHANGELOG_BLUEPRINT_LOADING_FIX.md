# CHANGELOG - Blueprint Loading Regression Fix

## Version 2.1.0 - 2026-01-09

### 🔴 Critical Bug Fix: Blueprint Loading Regression

#### Problem
After merging recent PRs, blueprints with schema v2 (using `"blueprintType"` field) stopped loading in the Blueprint Editor. This affected:
- All AI behavior trees (`Blueprints/AI/*.json`)
- All entity prefabs (`Blueprints/EntityPrefab/*.json`)
- All UI menus (`Blueprints/UI/*.json`)
- Animation graphs, level definitions, and scripted events

**Symptoms:**
- ❌ Double-clicking blueprints produced no action
- ❌ Asset Browser displayed files but without detected types
- ❌ Node Graph Editor remained empty with "No graph open"
- ❌ Inspector panel didn't show asset metadata

#### Root Cause
The C++ parser in `blueprinteditor.cpp` only checked for the `"type"` field, but schema v2 blueprints used `"blueprintType"` instead. This inconsistency prevented proper blueprint detection and loading.

**Schema Differences:**
- **Schema v1**: Uses `"type": "EntityBlueprint"` at root level
- **Schema v2**: Used `"blueprintType": "BehaviorTree"` at root level with data wrapper

---

### ✅ Solution Implemented

#### 1. C++ Parser Enhancements

**File**: `Source/BlueprintEditor/blueprinteditor.cpp`

**Changes to `DetectAssetType()` function:**
- ✅ Added priority-based type detection system
- ✅ Priority 1: Check explicit `"type"` field (standard)
- ✅ Priority 2: Fallback to `"blueprintType"` field (compatibility)
- ✅ Priority 3: Schema v2 structural detection (data wrapper)
- ✅ Priority 4: Schema v1 structural detection (direct fields)
- ✅ Added warning logs when deprecated `"blueprintType"` is used

**Changes to `ParseAssetMetadata()` function:**
- ✅ Implemented same priority-based detection
- ✅ Added support for all blueprint types (EntityPrefab, UIMenu, etc.)
- ✅ Added warning logs for migration guidance

**Changes to `ParseBehaviorTree()` function:**
- ✅ Added support for schema v2 `data` wrapper
- ✅ Checks `data.nodes` and `data.rootNodeId` for v2 files
- ✅ Maintains backward compatibility with v1 direct access

**Changes to `ParseEntityBlueprint()` function:**
- ✅ Added support for schema v2 `data` wrapper
- ✅ Checks `data.components` for v2 files
- ✅ Maintains backward compatibility with v1 direct access

#### 2. JSON Schema Standardization

**Standardized all schema v2 blueprints to include `"type"` field:**

**BehaviorTree files (4 files):**
- ✅ `Blueprints/AI/idle.json`
- ✅ `Blueprints/AI/investigate.json`
- ✅ `Blueprints/AI/guard_combat.json`
- ✅ `Blueprints/AI/guard_patrol.json`

**EntityPrefab files (6 files):**
- ✅ `Blueprints/EntityPrefab/player_entity.json`
- ✅ `Blueprints/EntityPrefab/npc_entity.json`
- ✅ `Blueprints/EntityPrefab/olympe_logo.json`
- ✅ `Blueprints/EntityPrefab/guard_npc.json`
- ✅ `Blueprints/EntityPrefab/waypoint.json`
- ✅ `Blueprints/EntityPrefab/trigger.json`

**Other blueprint types (5 files):**
- ✅ `Blueprints/UI/main_menu.json` (UIMenu)
- ✅ `Blueprints/UI/ingame_menu.json` (UIMenu)
- ✅ `Blueprints/Animations/guard_animations.json` (AnimationGraph)
- ✅ `Blueprints/Levels/tutorial_level.json` (LevelDefinition)
- ✅ `Blueprints/ScriptedEvents/guards_ambush.json` (ScriptedEvent)

**All files now include:**
```json
{
  "schema_version": 2,
  "type": "XXX",              // NEW - Primary identifier
  "blueprintType": "XXX",     // Kept for compatibility
  "name": "..."
}
```

#### 3. Documentation

**New file**: `BLUEPRINT_JSON_SCHEMA.md`
- ✅ Complete schema documentation for all blueprint types
- ✅ Required and optional fields for each type
- ✅ Migration guide from deprecated `"blueprintType"` to `"type"`
- ✅ Detection priority explanation
- ✅ Best practices and validation rules
- ✅ Example blueprints for each type

---

### 📊 Impact

**Fixed Files:**
- **C++**: 1 file modified (`blueprinteditor.cpp`)
- **JSON**: 15 files updated (added `"type"` field)
- **Documentation**: 2 files created/updated

**Backward Compatibility:**
- ✅ Schema v1 blueprints continue to work (example_entity_simple.json)
- ✅ Schema v2 blueprints with `"blueprintType"` only still work (with warning)
- ✅ Schema v2 blueprints with `"type"` preferred (no warning)

**Testing Results:**
- ✅ All blueprint types now detected correctly
- ✅ Asset Browser shows proper type labels
- ✅ Double-click opens blueprint in graph editor
- ✅ Inspector displays full metadata
- ✅ No breaking changes to existing functionality

---

### 🔄 Migration Path

#### For Existing Blueprints

**Step 1**: Add `"type"` field matching your `"blueprintType"`
```json
{
  "type": "BehaviorTree",
  "blueprintType": "BehaviorTree"  // Keep for now
}
```

**Step 2**: Test loading in Blueprint Editor

**Step 3**: Eventually remove `"blueprintType"` (future update)

#### For New Blueprints

Always include `"type"` field:
```json
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "name": "MyNewAI"
}
```

---

### ⚠️ Breaking Changes

**None** - This is a purely additive fix that maintains full backward compatibility.

---

### 🔍 Technical Details

#### Detection Priority System

1. **Explicit `type` field** (Highest priority)
   - Direct and unambiguous
   - Works for both schemas

2. **Fallback `blueprintType` field**
   - For compatibility with old v2 files
   - Logs deprecation warning

3. **Structural detection (v2)**
   - Checks `data.rootNodeId` + `data.nodes` → BehaviorTree
   - Checks `data.components` → EntityPrefab

4. **Structural detection (v1)**
   - Checks top-level `rootNodeId` + `nodes` → BehaviorTree
   - Checks top-level `components` → EntityBlueprint
   - Checks `states`/`initialState` → HFSM

5. **Generic fallback**
   - When no type can be determined

#### Warning Messages

When loading blueprints with only `"blueprintType"`:
```
[DetectAssetType] Warning: Using deprecated 'blueprintType' field, consider adding 'type' field to <filepath>
[ParseAssetMetadata] Warning: Using deprecated 'blueprintType' field in <filepath>
```

These warnings guide developers to migrate to the standardized `"type"` field.

---

### 📝 Related Issues

- Issue: Blueprint loading regression after PR merges
- Symptoms: Blueprints not loading, Asset Browser not showing types
- Affected: All schema v2 blueprints (AI, EntityPrefab, UI, etc.)

---

### 👥 Contributors

- **Atlasbruce** - Root cause analysis and issue documentation
- **GitHub Copilot** - Implementation and testing

---

### 📚 See Also

- **BLUEPRINT_JSON_SCHEMA.md** - Complete schema documentation
- **ARCHITECTURE.md** - Blueprint Editor architecture
- **HOTFIX_BLUEPRINT_LOADING.md** - Previous loading fixes
- **BlueprintMigrator.cpp** - Schema migration utilities

---

### 🎯 Future Improvements

1. **Schema v3 Planning**
   - Remove deprecated `"blueprintType"` field entirely
   - Simplify detection logic
   - Enhanced validation

2. **Migration Tools**
   - Automated script to add `"type"` to all files
   - Batch validation tool
   - Schema version upgrade tool

3. **Editor Enhancements**
   - Visual indicator for deprecated fields
   - Auto-fix suggestions
   - Schema validation on save

---

## Version History

### v2.1.0 (2026-01-09)
- **CRITICAL FIX**: Blueprint loading regression
- Added `"type"` field support with `"blueprintType"` fallback
- Updated all schema v2 blueprints with `"type"` field
- Created comprehensive JSON schema documentation
- Full backward compatibility maintained

### v2.0.0 (2026-01-08)
- Introduced schema v2 with `data` wrapper
- Enhanced metadata support
- Multiple editor improvements

### v1.0.0 (2025-XX-XX)
- Initial Blueprint Editor release
- Schema v1 implementation
- Basic component editing
