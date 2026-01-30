# Testing Guide: PrefabScanner Synonym System

## Overview

This guide covers testing the new unified PrefabScanner with JSON-based synonym registry system.

## What Was Implemented

### 1. JSON Synonym Registry (`Gamedata/EntityPrefab/EntityPrefabSynonymsRegister.json`)

A centralized configuration file that defines:
- 11 canonical entity types (Player, Guard, Enemy, etc.)
- Multiple synonyms per type (e.g., "player" → "Player", "guard" → "Guard")
- Fallback behavior configuration (case-insensitive, fuzzy matching)

### 2. Enhanced PrefabScanner

**New Features:**
- `Initialize()` - Unified initialization that loads synonyms and scans prefabs
- `NormalizeType()` - Converts any type string to canonical form
- `AreTypesEquivalent()` - Checks if two types are equivalent
- `ExtractPrefabType()` - Extracts type from `Identity_data::entityType` (priority fix)
- Fuzzy matching using Levenshtein distance algorithm

**Key Improvements:**
- Prefab type now correctly extracted from `Identity_data::entityType` component
- Case-insensitive type matching
- Recursive directory scanning (already existed)
- Detailed logging with ASCII borders

### 3. PrefabFactory Integration

**Changes:**
- `PreloadAllPrefabs()` now uses `PrefabScanner::Initialize()`
- Stores scanner instance for type normalization
- New wrapper methods: `NormalizeType()`, `AreTypesEquivalent()`

### 4. World.cpp Level Loading

**Changes:**
- Entity types are normalized after Phase 2 (prefab discovery)
- Pass 4 uses normalized types for prefab lookup
- Player registration uses `AreTypesEquivalent()` instead of string comparison

## Testing Scenarios

### Test 1: Basic Type Normalization

**Objective:** Verify that lowercase types from TMJ files are normalized to canonical form.

**Steps:**
1. Start the engine
2. Check startup logs for synonym registry loading
3. Load level: `Gamedata/Levels/isometric_quest.tmj`
4. Look for normalization logs in Phase 3

**Expected Logs:**
```
╔══════════════════════════════════════════════════════════╗
║ PREFAB SCANNER: INITIALIZATION                           ║
╚══════════════════════════════════════════════════════════╝
Directory: Gamedata/EntityPrefab

Step 1/3: Loading synonym registry...
  Loading: Gamedata/EntityPrefab/EntityPrefabSynonymsRegister.json
  ✅ Loaded 11 canonical types with 57 synonyms
  Settings: case-sensitive=no, fuzzy-matching=yes

[DEBUG] Normalizing entity types...
  → 'player' → 'Player'
  → 'guard' → 'Guard'
  → 'way' → 'Waypoint'
```

**Expected Result:**
- All entity types are normalized
- No red placeholder entities for player, guard, waypoint types

### Test 2: Prefab Type Extraction

**Objective:** Verify that prefab types are extracted from `Identity_data::entityType`.

**Steps:**
1. Check logs during prefab scanning
2. Verify each prefab shows its extracted type

**Expected Logs:**
```
Step 3/3: Parsing prefabs...
  ✅ Player [Player] (9 components)
  ✅ Guard [Guard] (13 components)
  ✅ Waypoint [Waypoint] (2 components)
```

**Verification:**
```bash
# Check player.json has correct Identity_data::entityType
cat Gamedata/EntityPrefab/player.json | grep -A 5 "Identity_data"
```

### Test 3: Player Entity Registration

**Objective:** Verify that Player entities from TMJ files are properly registered.

**Steps:**
1. Load `isometric_quest.tmj`
2. Check Pass 4 logs
3. Verify player registration

**Expected Logs:**
```
Pass 4: Dynamic Objects
  [Pass4] Creating: player_1 (type: Player)
  ✅ Created entity 1234567890 from 'Player' (9 components)
  ✅ Player 1 registered from level (Entity: 1234567890)
```

**Expected Result:**
- Player entity is visible (not a red placeholder)
- Player controls respond (WASD movement)

### Test 4: Synonym Variants

**Objective:** Test various synonym forms work correctly.

**Test Cases:**
```
Input Type       → Expected Canonical
─────────────────────────────────────
"player"         → "Player"
"PLAYER"         → "Player"
"playerentity"   → "Player"
"guard"          → "Guard"
"GUARD"          → "Guard"
"way"            → "Waypoint"
"WAY"            → "Waypoint"
"waypoint"       → "Waypoint"
"ambiant"        → "Ambient" (typo synonym)
"unknown_type"   → "unknown_type" (fallback)
```

### Test 5: Fuzzy Matching

**Objective:** Verify fuzzy matching works for typos (threshold: 0.8).

**Test Cases:**
```
Input Type    → Expected Match   Score
───────────────────────────────────────
"gaurd"       → "Guard"          ~0.83
"playr"       → "Player"         ~0.83
"waypoont"    → "Waypoint"       ~0.87
```

**Note:** Fuzzy matches should log:
```
🔍 Fuzzy match: 'gaurd' → 'Guard' (score: 0.83)
```

## Manual Testing Checklist

- [ ] Engine starts without errors
- [ ] Synonym registry loads successfully
- [ ] All 12 prefab files are scanned
- [ ] No invalid prefabs reported
- [ ] Load `isometric_quest.tmj` successfully
- [ ] Player entity is created (not red placeholder)
- [ ] Guard entities are created (not red placeholders)
- [ ] Waypoints are created
- [ ] Player controls work (WASD movement)
- [ ] No crashes during level loading

## Debug Commands

### View Synonym Registry
```bash
cat Gamedata/EntityPrefab/EntityPrefabSynonymsRegister.json | python3 -m json.tool
```

### Check Prefab Types
```bash
for f in Gamedata/EntityPrefab/*.json; do
  echo "=== $f ==="
  cat "$f" | grep -A 3 "entityType" | head -4
done
```

### Verify Level Entity Types
```bash
cat Gamedata/Levels/isometric_quest.tmj | grep '"type"' | grep -v '"object"' | sort | uniq
```

## Troubleshooting

### Problem: "Synonym registry not found"
**Solution:** Check file exists at `Gamedata/EntityPrefab/EntityPrefabSynonymsRegister.json`

### Problem: Entity types still not matching
**Solution:** 
1. Check logs for "Normalizing entity types..."
2. Verify synonym is in JSON file
3. Check case-sensitivity settings in JSON

### Problem: Red placeholder entities still appear
**Possible Causes:**
1. Prefab file missing/invalid
2. Type not in synonym registry
3. `Identity_data::entityType` missing in prefab JSON

**Debug Steps:**
```bash
# Check if prefab exists
ls -la Gamedata/EntityPrefab/player.json

# Check prefabType extraction
cat Gamedata/EntityPrefab/player.json | grep -B 2 -A 2 "entityType"

# Check logs for "Missing prefab for type:"
```

### Problem: Fuzzy matching not working
**Solution:** Check `fallbackBehavior` in JSON:
```json
"fallbackBehavior": {
  "enableFuzzyMatching": true,
  "fuzzyThreshold": 0.8
}
```

## Expected Performance

- Synonym registry loading: < 10ms
- Prefab scanning (12 files): < 100ms
- Type normalization (per entity): < 1ms
- Total startup overhead: < 200ms

## Success Criteria

✅ All tests pass
✅ No red placeholder entities for standard types
✅ Player entity is controllable
✅ Logs show proper normalization
✅ No performance regression

## Next Steps After Testing

1. Add more entity types to synonym registry as needed
2. Consider adding fuzzy match cache for performance
3. Add unit tests for normalization logic
4. Document custom type addition process for modders
