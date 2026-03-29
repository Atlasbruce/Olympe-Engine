---
id: troubleshooting
title: "Troubleshooting Guide"
sidebar_label: "Troubleshooting"
---

# Animation System Troubleshooting Guide

This guide covers common issues with the animation system and their solutions.

## Problem: Animations Don't Play

### Symptoms
- Entity appears with static sprite
- No frame changes occur
- Console shows no animation updates

### Possible Causes & Solutions

#### Cause 1: Animation Bank Not Loaded

**Check**:
```cpp
AnimationManager::Get().ListLoadedBanks();
```

**Solution**:
```cpp
// Ensure bank is loaded at startup
AnimationManager::Get().LoadAnimationBank("Gamedata/Animations/AnimationBanks/character.json");

// Or batch load
AnimationManager::Get().LoadAnimationBanksFromDirectory("Gamedata/Animations/AnimationBanks/");
```

**Prevention**: Add bank loading to game initialization code and verify console output shows successful loading.

#### Cause 2: Bank ID Mismatch

**Check**:
```cpp
// In prefab JSON:
"bankId": "player"  // Must match bank's "id" field

// In bank JSON:
"id": "player_animations"  // Different! Won't match
```

**Solution**: Ensure `bankId` in `VisualAnimation_data` exactly matches the `id` in the bank JSON file.

**Prevention**: Use consistent naming conventions and verify with `GetBank()`:
```cpp
auto bank = AnimationManager::Get().GetBank("player");
if (!bank) {
    std::cerr << "Bank 'player' not found!" << std::endl;
}
```

#### Cause 3: Animation Name Invalid

**Check**:
```cpp
// Component says:
"currentAnimName": "idle"

// But bank only has:
"animations": [
  { "name": "standing" },  // Different name!
  { "name": "walking" }
]
```

**Solution**: Ensure animation name exists in the bank. Check spelling and case-sensitivity.

**Prevention**: List available animations:
```cpp
auto bank = AnimationManager::Get().GetBank("character");
for (const auto& anim : bank->animations) {
    std::cout << "Available: " << anim.name << std::endl;
}
```

#### Cause 4: autoStart is False

**Check**:
```json
"VisualAnimation_data": {
  "autoStart": false  // Animation won't start automatically
}
```

**Solution**: Set `autoStart: true` or manually call:
```cpp
AnimationSystem::Get().PlayAnimation(entity, "idle", true);
```

**Prevention**: Use `autoStart: true` for animations that should play immediately.

---

## Problem: Wrong Frame Dimensions

### Symptoms
- Frames appear cropped or contain neighboring frames
- Animation shows parts of multiple characters
- Spacing looks incorrect

### Possible Causes & Solutions

#### Cause 1: Incorrect frameWidth/frameHeight

**Measure Spritesheet**:
1. Open spritesheet in image editor (Photoshop, GIMP, etc.)
2. Use selection tool to measure one frame
3. Note exact width and height in pixels

**Example**:
```
Measured: 64×64 pixels per frame
JSON says: "frameWidth": 32, "frameHeight": 32  // Wrong!
```

**Solution**: Update JSON with correct dimensions:
```json
"frameWidth": 64,
"frameHeight": 64
```

**Prevention**: Always measure spritesheets before creating bank JSON. Don't guess dimensions.

#### Cause 2: Incorrect Column Count

**Calculate Columns**:
```
Spritesheet width: 512 pixels
Frame width: 64 pixels
Columns = 512 / 64 = 8
```

**Check JSON**:
```json
"columns": 10  // Wrong! Should be 8
```

**Solution**: Update column count:
```json
"columns": 8
```

**Prevention**: Use formula: `columns = floor(spritesheetWidth / frameWidth)`

#### Cause 3: Spacing/Margin Not Accounted For

**Visual Inspection**:
```
If frames have gaps:
┌──┐ ┌──┐ ┌──┐  <- 2 pixel gaps
```

**Solution**: Add spacing value:
```json
"spacing": 2  // 2 pixels between frames
```

**For border padding**:
```json
"margin": 4  // 4 pixel border around grid
```

**Prevention**: Carefully inspect spritesheet layout. Look for gaps or borders.

---

## Problem: Animation Too Fast or Too Slow

### Symptoms
- Animation plays at wrong speed
- Character moves too quickly/slowly
- Frames flash by or linger too long

### Possible Causes & Solutions

#### Cause 1: frameDuration Too Small/Large

**Typical Durations**:
- **0.05s** (50ms): Very fast (20 FPS)
- **0.08s** (80ms): Fast (12.5 FPS)
- **0.1s** (100ms): Normal (10 FPS)
- **0.15s** (150ms): Slow (6.7 FPS)
- **0.2s** (200ms): Very slow (5 FPS)

**Solution**: Adjust `frameDuration` in bank JSON:
```json
"frameRange": {
  "start": 0,
  "end": 7,
  "frameDuration": 0.1  // Try different values: 0.05, 0.08, 0.12, 0.15
}
```

**Prevention**: Start with 0.1 seconds and adjust based on visual feedback.

#### Cause 2: Playback Speed Multiplier

**Check Component**:
```json
"playbackSpeed": 2.0  // 2× speed!
```

**Solution**: Reset to normal:
```json
"playbackSpeed": 1.0
```

Or adjust in code:
```cpp
AnimationSystem::Get().SetPlaybackSpeed(entity, 1.0f);
```

**Prevention**: Leave `playbackSpeed` at 1.0 unless intentionally creating slow-motion or fast-forward effects.

#### Cause 3: Frame Rate Issues

**Check Delta Time**:
```cpp
// In main game loop
std::cout << "Delta time: " << GameEngine::fDt << std::endl;
```

If delta time is very large (> 0.1), frames may skip.

**Solution**: Cap delta time:
```cpp
float cappedDt = std::min(GameEngine::fDt, 0.033f);  // Max 33ms per frame
```

**Prevention**: Ensure consistent frame rate (lock to 60 FPS if possible).

---

## Problem: Crash on Animation Switch

### Symptoms
- Game crashes when changing animations
- Segmentation fault or access violation
- Crash in `AnimationSystem::Process()`

### Possible Causes & Solutions

#### Cause 1: Animation Doesn't Exist in Bank

**Check**:
```cpp
AnimationSystem::Get().PlayAnimation(entity, "attack");
// But bank doesn't have "attack" animation
```

**Solution**: Verify animation exists:
```cpp
auto bank = AnimationManager::Get().GetBank("character");
auto anim = bank->GetAnimation("attack");
if (!anim) {
    std::cerr << "Animation 'attack' not found!" << std::endl;
    return;
}
AnimationSystem::Get().PlayAnimation(entity, "attack");
```

**Prevention**: Always check animation existence before playing, or ensure all referenced animations are defined in the bank.

#### Cause 2: Null Pointer to Animation Sequence

**Debug Output**:
```cpp
auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
if (!anim.currentSequence) {
    std::cerr << "currentSequence is null!" << std::endl;
}
```

**Solution**: Ensure bank and animation are loaded before accessing:
```cpp
// Always load banks first
AnimationManager::Get().LoadAnimationBank(...);

// Then create entities
PrefabFactory::Get().CreateEntityFromBlueprint(...);
```

**Prevention**: Load all animation resources during initialization, before creating any animated entities.

#### Cause 3: Entity Missing Required Components

**Check**:
```cpp
if (!World::Get().HasComponent<VisualAnimation_data>(entity)) {
    std::cerr << "Entity missing VisualAnimation_data!" << std::endl;
}
if (!World::Get().HasComponent<VisualSprite_data>(entity)) {
    std::cerr << "Entity missing VisualSprite_data!" << std::endl;
}
```

**Solution**: Ensure entity has both components:
```json
"components": {
  "VisualSprite_data": { ... },
  "VisualAnimation_data": { ... }
}
```

**Prevention**: Animation system requires both `VisualAnimation_data` and `VisualSprite_data` components.

---

## Problem: Texture Not Found

### Symptoms
- Console error: "Failed to load texture: ..."
- Black/missing sprite
- Pink placeholder sprite (if using error texture)

### Possible Causes & Solutions

#### Cause 1: Incorrect File Path

**Check Path Relative to Executable**:
```
Executable location: /project/build/OlympeEngine.exe
Texture path in JSON: "Gamedata/Sprites/hero.png"
Actual file location: /project/build/Gamedata/Sprites/hero.png  ✓
```

**Common Mistakes**:
```json
"spritesheetPath": "/Gamedata/Sprites/hero.png"  // Wrong: Absolute path
"spritesheetPath": "Gamedata\\Sprites\\hero.png" // Wrong: Backslashes (Windows-only)
"spritesheetPath": "Sprites/hero.png"            // Wrong: Missing Gamedata
```

**Solution**: Use correct relative path with forward slashes:
```json
"spritesheetPath": "Gamedata/Sprites/hero.png"
```

**Prevention**: 
- Always use forward slashes `/` (cross-platform)
- Always start paths relative to executable directory
- Test file paths with `ls` or `dir` commands

#### Cause 2: File Doesn't Exist

**Verify File Exists**:
```bash
ls -la Gamedata/Sprites/hero.png
# or on Windows:
dir Gamedata\Sprites\hero.png
```

**Solution**: Copy sprite file to correct location or update path in JSON.

**Prevention**: Organize assets in version control, ensure all team members have required files.

#### Cause 3: File Permissions

**Check Permissions** (Linux/Mac):
```bash
ls -l Gamedata/Sprites/hero.png
# Should show read permissions: -rw-r--r--
```

**Solution**: Fix permissions:
```bash
chmod 644 Gamedata/Sprites/hero.png
```

**Prevention**: Set correct permissions when adding files to project.

---

## Problem: Animation Stuttering

### Symptoms
- Animation appears jerky or inconsistent
- Some frames display longer than others
- Visual "hitching" during playback

### Possible Causes & Solutions

#### Cause 1: Inconsistent Frame Rate

**Check Frame Rate**:
```cpp
std::cout << "FPS: " << (1.0f / GameEngine::fDt) << std::endl;
```

If FPS varies wildly (20 → 60 → 30), animations will stutter.

**Solution**: 
- **V-Sync**: Enable V-Sync to lock frame rate to display refresh rate
- **Frame Rate Cap**: Limit to 60 FPS in game loop
- **Delta Time Smoothing**: Average delta time over several frames

```cpp
// Smooth delta time
static float dtHistory[10] = {0.016f};
static int dtIndex = 0;
dtHistory[dtIndex] = GameEngine::fDt;
dtIndex = (dtIndex + 1) % 10;
float smoothDt = 0.0f;
for (float dt : dtHistory) smoothDt += dt;
smoothDt /= 10.0f;
```

**Prevention**: Profile game performance, optimize rendering and physics.

#### Cause 2: Large Delta Time Spikes

**Check for Spikes**:
```cpp
if (GameEngine::fDt > 0.1f) {
    std::cerr << "Large delta time: " << GameEngine::fDt << std::endl;
}
```

**Solution**: Cap delta time:
```cpp
float cappedDt = std::min(GameEngine::fDt, 0.033f);  // Max 33ms
```

**Prevention**: Avoid blocking operations in game loop (file I/O, network, heavy computations).

#### Cause 3: frameDuration Not Multiple of Frame Time

**Example**:
- Frame time: 16.67ms (60 FPS)
- frameDuration: 0.09s = 90ms = 5.4 frames

This causes inconsistent rounding.

**Solution**: Use frameDuration that's a multiple of frame time:
```
60 FPS = 16.67ms per frame
Good durations: 0.0167s (1 frame), 0.033s (2 frames), 0.05s (3 frames), 0.0667s (4 frames)
```

**Prevention**: Choose frame durations that divide evenly into your target frame rate.

---

## Problem: Animation Doesn't Loop

### Symptoms
- Animation plays once and stops
- Character freezes on last frame
- Expected looping doesn't occur

### Possible Causes & Solutions

#### Cause 1: loop Set to false

**Check JSON**:
```json
"animations": [
  {
    "name": "idle",
    "loop": false  // Wrong for idle!
  }
]
```

**Solution**: Set loop to true:
```json
"loop": true
```

**Prevention**: Use `loop: true` for idle, walk, run animations. Use `loop: false` for attack, death, jump.

#### Cause 2: isPlaying Set to false

**Check Component**:
```json
"VisualAnimation_data": {
  "isPlaying": false  // Animation paused
}
```

**Solution**: Set isPlaying to true or call:
```cpp
AnimationSystem::Get().ResumeAnimation(entity);
```

**Prevention**: Use `isPlaying: true` by default.

#### Cause 3: nextAnimation Triggers

**Check JSON**:
```json
{
  "name": "idle",
  "loop": false,
  "nextAnimation": "walk"  // Switches to walk after one loop
}
```

**Solution**: Remove `nextAnimation` for looping animations:
```json
{
  "name": "idle",
  "loop": true
  // No nextAnimation
}
```

**Prevention**: Only use `nextAnimation` with `loop: false`.

---

## Problem: Wrong Frames Displayed

### Symptoms
- Animation shows wrong part of spritesheet
- Frames appear in wrong order
- Character displays incorrect sprites

### Possible Causes & Solutions

#### Cause 1: Incorrect Frame Calculation

**Debug Frame Index**:
```cpp
auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
std::cout << "Frame " << anim.currentFrame << " of " << anim.currentAnimName << std::endl;

// Calculate srcRect manually
int row = anim.currentFrame / columns;
int col = anim.currentFrame % columns;
SDL_Rect rect = {
    col * frameWidth + margin + col * spacing,
    row * frameHeight + margin + row * spacing,
    frameWidth,
    frameHeight
};
std::cout << "Expected srcRect: " << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << std::endl;
```

Compare with actual `sprite.srcRect`.

**Solution**: Verify `columns`, `spacing`, and `margin` values in bank JSON match spritesheet layout.

#### Cause 2: frameRange start/end Wrong

**Check JSON**:
```json
"frameRange": {
  "start": 10,
  "end": 5  // Wrong! start > end
}
```

**Solution**: Swap start and end:
```json
"frameRange": {
  "start": 5,
  "end": 10
}
```

**Prevention**: Always ensure `start <= end`. The system should validate this, but double-check.

#### Cause 3: Using Wrong Row

**Frame Indexing**:
```
8 columns per row:
Row 0: Frames 0-7
Row 1: Frames 8-15
Row 2: Frames 16-23

Want row 2 (frames 16-23) but specified 8-15 (row 1)
```

**Solution**: Calculate correct frame range:
```
Row index × columns = start frame
Example: Row 2 × 8 = Frame 16
```

**Prevention**: Use formula `startFrame = rowIndex × columns`.

---

## Problem: Animation Events Not Triggering

### Symptoms
- Footstep sounds don't play
- Frame-specific effects missing
- Events defined in JSON but not occurring

### Possible Causes & Solutions

#### Cause 1: Using frameRange Instead of frames

**Issue**: Frame events require explicit `frames` array, not `frameRange`.

**Wrong**:
```json
"frameRange": {
  "start": 0,
  "end": 7,
  "frameDuration": 0.1
}
// No way to add events per frame
```

**Solution**: Use explicit frames:
```json
"frames": [
  { "srcRect": {...}, "duration": 0.1, "events": [] },
  { "srcRect": {...}, "duration": 0.1, "events": [] },
  { "srcRect": {...}, "duration": 0.1, "events": ["footstep"] },
  ...
]
```

**Prevention**: Use `frames` array whenever you need frame-specific data (events, hot spots, variable durations).

#### Cause 2: Event Handler Not Implemented

**Check**: Do you have code listening for animation events?

**Solution**: Implement event handling:
```cpp
void AnimationSystem::TriggerFrameEvents(ECS_Entity entity, int frameIndex)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    const auto& frame = anim.currentSequence->frames[frameIndex];
    
    for (const std::string& event : frame.events)
    {
        // Trigger event
        EventManager::Get().TriggerEvent("animation_event", entity, event);
    }
}
```

**Prevention**: Ensure `AnimationSystem` calls event handlers on frame changes.

#### Cause 3: Events on Wrong Frames

**Debug**:
```cpp
// Log all frame events
for (int i = 0; i < anim.currentSequence->frames.size(); ++i)
{
    const auto& frame = anim.currentSequence->frames[i];
    std::cout << "Frame " << i << " events: ";
    for (const auto& evt : frame.events)
        std::cout << evt << " ";
    std::cout << std::endl;
}
```

**Solution**: Move events to correct frames in JSON.

**Prevention**: Watch animation frame-by-frame to identify correct event frames.

---

## Problem: Memory Leaks

### Symptoms
- Memory usage grows over time
- Game slows down after running for a while
- Profiler shows animation system leaking memory

### Possible Causes & Solutions

#### Cause 1: Not Unloading Banks

**Issue**: Loading banks repeatedly without clearing.

**Solution**: Clear banks when changing levels:
```cpp
AnimationManager::Get().UnloadBank("level1_enemies");
AnimationManager::Get().UnloadBank("level1_bosses");
```

Or clear all:
```cpp
AnimationManager::Get().UnloadAllBanks();
```

**Prevention**: Profile memory usage, unload unused resources.

#### Cause 2: Dangling Entity References

**Issue**: Entities destroyed but animation system still references them.

**Solution**: Ensure animation system removes destroyed entities:
```cpp
// In AnimationSystem::Process()
for (ECS_Entity entity : m_entities)
{
    if (!World::Get().IsEntityValid(entity))
    {
        m_entities.erase(entity);
        continue;
    }
    // ...
}
```

**Prevention**: Use proper entity lifecycle management, remove entities from systems when destroyed.

---

## Diagnostic Commands

### Enable Verbose Logging

```cpp
AnimationManager::Get().SetVerbose(true);
AnimationSystem::Get().SetDebugMode(true);
```

### List Loaded Resources

```cpp
// List all banks
AnimationManager::Get().ListLoadedBanks();

// List all graphs
AnimationManager::Get().ListLoadedGraphs();
```

### Dump Component State

```cpp
void DumpAnimationState(ECS_Entity entity)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    std::cout << "=== Animation State ===" << std::endl;
    std::cout << "Bank ID: " << anim.bankId << std::endl;
    std::cout << "Graph ID: " << anim.graphId << std::endl;
    std::cout << "Current Anim: " << anim.currentAnimName << std::endl;
    std::cout << "Current Frame: " << anim.currentFrame << std::endl;
    std::cout << "Is Playing: " << anim.isPlaying << std::endl;
    std::cout << "Loop: " << anim.loop << std::endl;
    std::cout << "Playback Speed: " << anim.playbackSpeed << std::endl;
    std::cout << "======================" << std::endl;
}
```

### Validate Animation Bank

```cpp
bool ValidateBank(const std::string& bankId)
{
    auto bank = AnimationManager::Get().GetBank(bankId);
    if (!bank)
    {
        std::cerr << "Bank '" << bankId << "' not found" << std::endl;
        return false;
    }
    
    std::cout << "Bank: " << bank->id << std::endl;
    std::cout << "Spritesheet: " << bank->spritesheetPath << std::endl;
    std::cout << "Animations: " << bank->animations.size() << std::endl;
    
    for (const auto& anim : bank->animations)
    {
        std::cout << "  - " << anim.name 
                  << " (" << anim.frames.size() << " frames, "
                  << (anim.loop ? "looping" : "one-shot") << ")" << std::endl;
    }
    
    return true;
}
```

---

## Getting More Help

If you've tried all solutions and still have issues:

1. **Check Console Logs**: Look for error messages from AnimationManager/AnimationSystem
2. **Verify JSON Syntax**: Use a JSON validator tool
3. **Test with Simple Example**: Try the Thésée tutorial to verify system works
4. **Community Support**: Ask on Discord or GitHub Discussions
5. **File a Bug Report**: Open an issue on GitHub with:
   - Detailed description
   - JSON files (bank, graph, prefab)
   - Console output
   - Steps to reproduce

## See Also

- [Quick Start Guide](animation-system-quick-start.md) - Verify basic setup
- [Animation Banks Reference](animation-banks.md) - Check JSON format
- [Animation Graphs Reference](animation-graphs.md) - Validate FSM structure
- [API Reference](../../technical-reference/animation/api-reference.md) - Understand API usage
