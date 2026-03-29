---
id: api-reference
title: "Animation API Reference"
sidebar_label: "API Reference"
---

# Animation API Reference

Complete API documentation for the Olympe Engine Animation System.

## AnimationManager

Singleton class responsible for loading and managing animation banks and graphs.

### Header
```cpp
#include "Animation/AnimationManager.h"
```

### Access Singleton
```cpp
AnimationManager& manager = AnimationManager::Get();
```

---

### LoadAnimationBank

Load a single animation bank from JSON file.

**Signature**:
```cpp
bool LoadAnimationBank(const std::string& bankJsonPath);
```

**Parameters**:
- `bankJsonPath`: Relative path to animation bank JSON file

**Returns**:
- `true` if loaded successfully
- `false` if file not found or JSON invalid

**Example**:
```cpp
bool success = AnimationManager::Get().LoadAnimationBank(
    "Gamedata/Animations/AnimationBanks/player.json"
);

if (success) {
    std::cout << "Player animations loaded" << std::endl;
} else {
    std::cerr << "Failed to load player animations" << std::endl;
}
```

**Errors**:
- File not found
- Invalid JSON syntax
- Missing required fields (id, spritesheetPath, frameWidth, etc.)
- Duplicate bank ID

**See Also**: LoadAnimationBanksFromDirectory

---

### LoadAnimationBanksFromDirectory

Batch load all animation banks from a directory.

**Signature**:
```cpp
int LoadAnimationBanksFromDirectory(const std::string& directoryPath);
```

**Parameters**:
- `directoryPath`: Path to directory containing `.json` files

**Returns**:
- Number of banks successfully loaded

**Example**:
```cpp
int count = AnimationManager::Get().LoadAnimationBanksFromDirectory(
    "Gamedata/Animations/AnimationBanks/"
);

std::cout << "Loaded " << count << " animation banks" << std::endl;
```

**Behavior**:
- Scans directory for `.json` files
- Attempts to load each file as animation bank
- Logs success/failure for each file
- Continues on error (doesn't stop at first failure)

**Common Use Case**: Load all game animations at startup.

---

### LoadAnimationGraph

Load a single animation graph (FSM) from JSON file.

**Signature**:
```cpp
bool LoadAnimationGraph(const std::string& graphJsonPath);
```

**Parameters**:
- `graphJsonPath`: Relative path to animation graph JSON file

**Returns**:
- `true` if loaded successfully
- `false` if file not found or JSON invalid

**Example**:
```cpp
bool success = AnimationManager::Get().LoadAnimationGraph(
    "Gamedata/Animations/AnimationGraphs/player_fsm.json"
);

if (success) {
    std::cout << "Player FSM loaded" << std::endl;
}
```

**Errors**:
- File not found
- Invalid JSON syntax
- Missing required fields (id, defaultState, states)
- Invalid state transitions

**Validation**:
- Ensures `defaultState` references a valid state
- Validates all `transitions.to` references
- Warns about unreachable states

---

### LoadAnimationGraphsFromDirectory

Batch load all animation graphs from a directory.

**Signature**:
```cpp
int LoadAnimationGraphsFromDirectory(const std::string& directoryPath);
```

**Parameters**:
- `directoryPath`: Path to directory containing `.json` files

**Returns**:
- Number of graphs successfully loaded

**Example**:
```cpp
int count = AnimationManager::Get().LoadAnimationGraphsFromDirectory(
    "Gamedata/Animations/AnimationGraphs/"
);

std::cout << "Loaded " << count << " animation graphs" << std::endl;
```

---

### GetBank

Retrieve an animation bank by ID.

**Signature**:
```cpp
std::shared_ptr<AnimationBank> GetBank(const std::string& bankId) const;
```

**Parameters**:
- `bankId`: Unique identifier of the bank (from JSON `"id"` field)

**Returns**:
- `shared_ptr` to AnimationBank if found
- `nullptr` if bank not loaded

**Example**:
```cpp
auto bank = AnimationManager::Get().GetBank("player");

if (bank) {
    std::cout << "Bank has " << bank->animations.size() << " animations" << std::endl;
} else {
    std::cerr << "Bank 'player' not found" << std::endl;
}
```

**Use Cases**:
- Verify bank is loaded before creating entities
- List available animations in bank
- Debug resource loading issues

---

### GetGraph

Retrieve an animation graph by ID.

**Signature**:
```cpp
std::shared_ptr<AnimationGraph> GetGraph(const std::string& graphId) const;
```

**Parameters**:
- `graphId`: Unique identifier of the graph (from JSON `"id"` field)

**Returns**:
- `shared_ptr` to AnimationGraph if found
- `nullptr` if graph not loaded

**Example**:
```cpp
auto graph = AnimationManager::Get().GetGraph("player_fsm");

if (graph) {
    std::cout << "Graph has " << graph->states.size() << " states" << std::endl;
    std::cout << "Default state: " << graph->defaultState << std::endl;
}
```

---

### ListLoadedBanks

Print all loaded animation banks to console (debug).

**Signature**:
```cpp
void ListLoadedBanks() const;
```

**Example**:
```cpp
AnimationManager::Get().ListLoadedBanks();
```

**Output**:
```
[AnimationManager] Loaded Animation Banks:
  - player (4 animations)
  - enemy_goblin (6 animations)
  - boss_dragon (12 animations)
```

**Use Cases**:
- Verify banks loaded at startup
- Debug missing animations
- Check resource loading order

---

### ListLoadedGraphs

Print all loaded animation graphs to console (debug).

**Signature**:
```cpp
void ListLoadedGraphs() const;
```

**Example**:
```cpp
AnimationManager::Get().ListLoadedGraphs();
```

**Output**:
```
[AnimationManager] Loaded Animation Graphs:
  - player_fsm (8 states, default: idle)
  - enemy_fsm (5 states, default: patrol)
```

---

### UnloadBank

Unload an animation bank (free memory).

**Signature**:
```cpp
bool UnloadBank(const std::string& bankId);
```

**Parameters**:
- `bankId`: ID of bank to unload

**Returns**:
- `true` if bank was found and unloaded
- `false` if bank not found

**Example**:
```cpp
// Unload level-specific animations
AnimationManager::Get().UnloadBank("level1_enemies");
AnimationManager::Get().UnloadBank("level1_bosses");
```

**Warning**: Entities referencing unloaded banks will have invalid pointers. Ensure no entities use the bank before unloading.

---

### UnloadAllBanks

Unload all animation banks.

**Signature**:
```cpp
void UnloadAllBanks();
```

**Example**:
```cpp
// Clean up when exiting game
AnimationManager::Get().UnloadAllBanks();
```

---

## AnimationSystem

ECS system that updates animation frames each tick.

### Header
```cpp
#include "ECS_Systems_Animation.h"
```

### Access Singleton
```cpp
AnimationSystem& system = AnimationSystem::Get();
```

---

### PlayAnimation

Start or switch to a different animation.

**Signature**:
```cpp
bool PlayAnimation(ECS_Entity entity, 
                   const std::string& animationName, 
                   bool restart = false);
```

**Parameters**:
- `entity`: Entity with VisualAnimation_data component
- `animationName`: Name of animation to play (from bank)
- `restart`: If true, restart animation from frame 0 even if already playing

**Returns**:
- `true` if animation started successfully
- `false` if entity invalid, component missing, or animation not found

**Example 1: Basic Animation Switch**
```cpp
// Switch to walk animation (doesn't restart if already walking)
AnimationSystem::Get().PlayAnimation(player, "walk", false);

// Switch to run animation
AnimationSystem::Get().PlayAnimation(player, "run", false);
```

**Example 2: Restart Animation**
```cpp
// Always restart attack animation from beginning
if (InputManager::Get().IsActionJustPressed("attack")) {
    AnimationSystem::Get().PlayAnimation(player, "attack", true);
}
```

**Example 3: Conditional Animation**
```cpp
void UpdateMovementAnimation(ECS_Entity entity, float speed)
{
    if (speed < 10.0f) {
        AnimationSystem::Get().PlayAnimation(entity, "idle", false);
    } else if (speed < 200.0f) {
        AnimationSystem::Get().PlayAnimation(entity, "walk", false);
    } else {
        AnimationSystem::Get().PlayAnimation(entity, "run", false);
    }
}
```

**Behavior**:
- Stops current animation
- Sets new animation as current
- Resets frame to 0
- Resets elapsed time to 0
- Starts playback immediately

---

### PauseAnimation

Pause animation playback (can be resumed).

**Signature**:
```cpp
void PauseAnimation(ECS_Entity entity);
```

**Parameters**:
- `entity`: Entity to pause

**Example**:
```cpp
// Pause game
if (InputManager::Get().IsActionJustPressed("pause")) {
    for (ECS_Entity entity : allEntities) {
        AnimationSystem::Get().PauseAnimation(entity);
    }
}
```

**Behavior**:
- Sets `isPlaying = false`
- Preserves current frame
- Preserves elapsed time
- No visual change (stays on current frame)

---

### ResumeAnimation

Resume paused animation playback.

**Signature**:
```cpp
void ResumeAnimation(ECS_Entity entity);
```

**Parameters**:
- `entity`: Entity to resume

**Example**:
```cpp
// Unpause game
if (InputManager::Get().IsActionJustPressed("unpause")) {
    for (ECS_Entity entity : allEntities) {
        AnimationSystem::Get().ResumeAnimation(entity);
    }
}
```

**Behavior**:
- Sets `isPlaying = true`
- Continues from current frame
- No reset (unlike PlayAnimation)

---

### StopAnimation

Stop animation and reset to first frame.

**Signature**:
```cpp
void StopAnimation(ECS_Entity entity);
```

**Parameters**:
- `entity`: Entity to stop

**Example**:
```cpp
// Stop all animations on entity death
void OnEntityDeath(ECS_Entity entity)
{
    AnimationSystem::Get().StopAnimation(entity);
    // Entity displays first frame of last animation
}
```

**Behavior**:
- Sets `isPlaying = false`
- Resets `currentFrame = 0`
- Resets `elapsedTime = 0.0f`
- Displays first frame of current animation

**Difference from Pause**: Stop resets to frame 0, pause preserves frame.

---

### SetPlaybackSpeed

Change animation playback speed multiplier.

**Signature**:
```cpp
void SetPlaybackSpeed(ECS_Entity entity, float speed);
```

**Parameters**:
- `entity`: Entity to modify
- `speed`: Speed multiplier (1.0 = normal, 2.0 = double speed, 0.5 = half speed)

**Example 1: Slow Motion**
```cpp
// Apply slow motion effect
AnimationSystem::Get().SetPlaybackSpeed(entity, 0.5f);
```

**Example 2: Fast Forward**
```cpp
// Speed up idle animation for quick NPC
AnimationSystem::Get().SetPlaybackSpeed(npc, 1.5f);
```

**Example 3: Time-Based Effects**
```cpp
// Apply haste buff
void ApplyHasteBuff(ECS_Entity entity)
{
    AnimationSystem::Get().SetPlaybackSpeed(entity, 2.0f);
}

// Remove haste buff
void RemoveHasteBuff(ECS_Entity entity)
{
    AnimationSystem::Get().SetPlaybackSpeed(entity, 1.0f);
}
```

**Behavior**:
- Multiplies frame duration by speed
- Speed = 2.0 → frames advance twice as fast
- Speed = 0.5 → frames advance half as fast
- Speed = 0.0 → effectively paused (not recommended, use PauseAnimation)

**Note**: This multiplies the `speed` value in the animation bank JSON.

---

### TransitionToState

Transition to a new state in the animation graph (FSM).

**Signature**:
```cpp
bool TransitionToState(ECS_Entity entity, const std::string& stateName);
```

**Parameters**:
- `entity`: Entity with VisualAnimation_data component
- `stateName`: Name of target state in animation graph

**Returns**:
- `true` if transition succeeded
- `false` if transition invalid (not in graph transitions)

**Example**:
```cpp
void HandleCombatInput(ECS_Entity player)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(player);
    
    if (InputManager::Get().IsActionJustPressed("attack"))
    {
        // FSM validates this transition
        if (AnimationSystem::Get().TransitionToState(player, "attack"))
        {
            std::cout << "Started attack" << std::endl;
        }
        else
        {
            std::cout << "Cannot attack from " << anim.currentStateName << std::endl;
        }
    }
}
```

**Behavior**:
- Checks current state's valid transitions
- If valid, changes to new state
- Plays animation associated with new state
- Updates `currentStateName` field

**Validation**: Only transitions listed in current state's `transitions` array are allowed.

---

### GetCurrentFrame

Get current frame index of entity's animation.

**Signature**:
```cpp
int GetCurrentFrame(ECS_Entity entity) const;
```

**Parameters**:
- `entity`: Entity to query

**Returns**:
- Current frame index (0-based)
- -1 if entity invalid or no animation

**Example**:
```cpp
int frame = AnimationSystem::Get().GetCurrentFrame(player);
std::cout << "Player on frame " << frame << std::endl;

// Check if on specific frame
if (frame == 3) {
    PlaySound("footstep.wav");
}
```

---

### IsAnimationComplete

Check if non-looping animation has finished.

**Signature**:
```cpp
bool IsAnimationComplete(ECS_Entity entity) const;
```

**Parameters**:
- `entity`: Entity to query

**Returns**:
- `true` if animation finished (non-looping, on last frame, stopped)
- `false` if still playing or looping

**Example**:
```cpp
void CheckAttackComplete(ECS_Entity entity)
{
    if (AnimationSystem::Get().IsAnimationComplete(entity))
    {
        std::cout << "Attack animation finished" << std::endl;
        AnimationSystem::Get().PlayAnimation(entity, "idle", true);
    }
}
```

---

## VisualAnimation_data Component

ECS component storing animation state.

### Header
```cpp
#include "ECS_Components.h"
```

### Structure

```cpp
struct VisualAnimation_data
{
    std::string bankId;              // Animation bank identifier
    std::string graphId;             // Animation graph identifier (optional)
    std::string currentAnimName;     // Current animation name
    std::string currentStateName;    // Current FSM state name
    int currentFrame;                // Current frame index
    float elapsedTime;               // Time since last frame change
    bool isPlaying;                  // Whether animation is playing
    bool loop;                       // Whether animation loops
    float playbackSpeed;             // Speed multiplier
    bool autoStart;                  // Start playing on entity creation
    AnimationSequence* currentSequence;  // Pointer to current sequence
    SDL_Texture* cachedTexture;      // Cached texture pointer
    bool isAnimationComplete;        // True when non-looping anim finishes
    int totalFrames;                 // Total frame count
};
```

### Field Reference

#### bankId
- **Type**: `std::string`
- **Purpose**: Links to AnimationBank in AnimationManager
- **Set**: At entity creation (prefab JSON)
- **Modify**: Rarely (only if changing character's entire animation set)

#### graphId
- **Type**: `std::string`
- **Purpose**: Links to AnimationGraph for FSM control
- **Optional**: Leave empty if not using FSM
- **Set**: At entity creation

#### currentAnimName
- **Type**: `std::string`
- **Purpose**: Name of current animation (e.g., "walk", "attack")
- **Set**: By AnimationSystem::PlayAnimation()
- **Read**: For debugging or conditional logic

#### currentFrame
- **Type**: `int`
- **Purpose**: Current frame index in animation sequence (0-based)
- **Set**: By AnimationSystem::Process()
- **Read**: For frame-specific logic or debugging

#### elapsedTime
- **Type**: `float` (seconds)
- **Purpose**: Time accumulated since last frame change
- **Set**: By AnimationSystem::Process()
- **Rarely Modified**: Internal state

#### isPlaying
- **Type**: `bool`
- **Purpose**: Whether animation is actively updating
- **Set**: By PlayAnimation(), PauseAnimation(), ResumeAnimation()
- **Modify**: To manually pause/resume

#### loop
- **Type**: `bool`
- **Purpose**: Whether animation repeats or plays once
- **Set**: From animation bank JSON
- **Modify**: To dynamically change loop behavior

#### playbackSpeed
- **Type**: `float`
- **Purpose**: Speed multiplier (1.0 = normal)
- **Set**: By SetPlaybackSpeed()
- **Modify**: For slow-motion, fast-forward, buffs/debuffs

#### autoStart
- **Type**: `bool`
- **Purpose**: Start playing immediately on entity creation
- **Set**: At entity creation (prefab JSON)
- **Typical**: `true` for most entities

### Usage Patterns

#### Pattern 1: Create Entity with Animation
```cpp
ECS_Entity CreateAnimatedEntity()
{
    ECS_Entity entity = World::Get().CreateEntity();
    
    VisualSprite_data sprite;
    sprite.texturePath = "Gamedata/Sprites/hero.png";
    sprite.srcRect = {0, 0, 64, 64};
    sprite.dstRect = {0, 0, 64, 64};
    World::Get().AddComponent<VisualSprite_data>(entity, sprite);
    
    VisualAnimation_data anim;
    anim.bankId = "hero";
    anim.graphId = "hero_fsm";
    anim.currentAnimName = "idle";
    anim.isPlaying = true;
    anim.autoStart = true;
    World::Get().AddComponent<VisualAnimation_data>(entity, anim);
    
    return entity;
}
```

#### Pattern 2: Query Animation State
```cpp
void CheckAnimationState(ECS_Entity entity)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    
    std::cout << "Bank: " << anim.bankId << std::endl;
    std::cout << "Animation: " << anim.currentAnimName << std::endl;
    std::cout << "Frame: " << anim.currentFrame << "/" << anim.totalFrames << std::endl;
    std::cout << "Playing: " << (anim.isPlaying ? "Yes" : "No") << std::endl;
}
```

#### Pattern 3: Modify Playback
```cpp
void ApplyStatusEffect(ECS_Entity entity, const std::string& effect)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    
    if (effect == "frozen") {
        anim.playbackSpeed = 0.2f;  // Very slow
    } else if (effect == "haste") {
        anim.playbackSpeed = 2.0f;  // Double speed
    } else {
        anim.playbackSpeed = 1.0f;  // Normal
    }
}
```

### Common Pitfalls

#### Pitfall 1: Null Pointer Dereference
```cpp
// DON'T: Access currentSequence without checking
auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
int frameCount = anim.currentSequence->frames.size();  // Crash if null!

// DO: Check for null
if (anim.currentSequence) {
    int frameCount = anim.currentSequence->frames.size();
}
```

#### Pitfall 2: Modifying Bank/Graph IDs at Runtime
```cpp
// DON'T: Change bankId after entity created
anim.bankId = "different_bank";  // currentSequence pointer now invalid!

// DO: Create new entity or reload animation
AnimationSystem::Get().PlayAnimation(entity, "new_animation", true);
```

#### Pitfall 3: Forgetting autoStart
```cpp
// Animation won't play if autoStart = false and never manually started
VisualAnimation_data anim;
anim.bankId = "hero";
anim.currentAnimName = "idle";
anim.autoStart = false;  // Oops! Animation frozen
```

## Code Examples

### Example 1: Full Startup Initialization

```cpp
void Game::Init()
{
    // Load all animation resources
    AnimationManager::Get().LoadAnimationBanksFromDirectory(
        "Gamedata/Animations/AnimationBanks/"
    );
    AnimationManager::Get().LoadAnimationGraphsFromDirectory(
        "Gamedata/Animations/AnimationGraphs/"
    );
    
    // Verify loading
    AnimationManager::Get().ListLoadedBanks();
    AnimationManager::Get().ListLoadedGraphs();
    
    // Create animated entities
    m_player = PrefabFactory::Get().CreateEntityFromBlueprint(
        "Gamedata/Blueprints/Characters/player.json"
    );
}
```

### Example 2: State-Based Animation Control

```cpp
void UpdateEnemyAnimation(ECS_Entity enemy)
{
    auto& ai = World::Get().GetComponent<AI_data>(enemy);
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(enemy);
    
    // Update animation based on AI state
    if (ai.currentState == "PATROL") {
        if (anim.currentAnimName != "walk") {
            AnimationSystem::Get().PlayAnimation(enemy, "walk", false);
        }
    } else if (ai.currentState == "CHASE") {
        if (anim.currentAnimName != "run") {
            AnimationSystem::Get().PlayAnimation(enemy, "run", false);
        }
    } else if (ai.currentState == "ATTACK") {
        if (anim.currentAnimName != "attack") {
            AnimationSystem::Get().PlayAnimation(enemy, "attack", true);
        }
    } else {
        if (anim.currentAnimName != "idle") {
            AnimationSystem::Get().PlayAnimation(enemy, "idle", false);
        }
    }
}
```

### Example 3: Frame-Specific Logic

```cpp
void CheckAttackHitFrame(ECS_Entity attacker)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(attacker);
    
    // Check if on hit frame (frame 3 of attack animation)
    if (anim.currentAnimName == "attack" && anim.currentFrame == 3)
    {
        // Deal damage
        CheckHitboxCollision(attacker);
        PlaySound("sword_hit.wav");
    }
}
```

## See Also

- [Quick Start Guide](../../user-guide/animation-system/animation-system-quick-start.md) - Tutorial
- [Animation Banks Reference](../../user-guide/animation-system/animation-banks.md) - JSON format
- [Animation Graphs Reference](../../user-guide/animation-system/animation-graphs.md) - FSM configuration
- [System Architecture](animation-system.md) - Technical deep-dive
