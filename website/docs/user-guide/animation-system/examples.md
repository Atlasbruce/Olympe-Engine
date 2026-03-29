---
id: examples
title: "Animation Examples Gallery"
sidebar_label: "Examples"
---

# Animation Examples Gallery

This gallery provides practical examples for common animation scenarios. Each example includes the use case, JSON configuration, C++ control code, and expected behavior.

## Example 1: Simple Idle Animation

### Use Case
A static enemy or NPC with a breathing/swaying idle animation that loops continuously.

### Animation Bank JSON

```json
{
  "id": "simple_npc",
  "spritesheetPath": "Gamedata/Sprites/NPCs/guard.png",
  "frameWidth": 48,
  "frameHeight": 64,
  "columns": 6,
  "spacing": 0,
  "margin": 0,
  "animations": [
    {
      "name": "idle",
      "loop": true,
      "speed": 1.0,
      "frameRange": {
        "start": 0,
        "end": 5,
        "frameDuration": 0.15
      }
    }
  ]
}
```

### Entity Prefab

```json
{
  "version": "1.0",
  "name": "Guard NPC",
  "components": {
    "VisualSprite_data": {
      "texturePath": "Gamedata/Sprites/NPCs/guard.png",
      "srcRect": { "x": 0, "y": 0, "w": 48, "h": 64 },
      "dstRect": { "x": 0, "y": 0, "w": 48, "h": 64 },
      "drawLayer": 5,
      "visible": true
    },
    "VisualAnimation_data": {
      "bankId": "simple_npc",
      "currentAnimName": "idle",
      "isPlaying": true,
      "loop": true,
      "autoStart": true
    },
    "Transform_data": {
      "position": { "x": 320, "y": 240 }
    }
  }
}
```

### Expected Behavior
- NPC breathes/sways gently
- Animation loops seamlessly (frame 5 → frame 0)
- Total cycle time: 0.9 seconds (6 frames × 0.15s)

---

## Example 2: Walk Cycle with 8 Directions

### Use Case
Top-down game character with 8-directional walking animations (N, NE, E, SE, S, SW, W, NW).

### Spritesheet Layout

```
Spritesheet: 512×512 (8 columns × 8 rows)
Frame size: 64×64

Row 0: Walk South     (frames 0-7)
Row 1: Walk South-East (frames 8-15)
Row 2: Walk East      (frames 16-23)
Row 3: Walk North-East (frames 24-31)
Row 4: Walk North     (frames 32-39)
Row 5: Walk North-West (frames 40-47)
Row 6: Walk West      (frames 48-55)
Row 7: Walk South-West (frames 56-63)
```

### Animation Bank JSON

```json
{
  "id": "character_8dir",
  "spritesheetPath": "Gamedata/Sprites/Characters/hero_8dir.png",
  "frameWidth": 64,
  "frameHeight": 64,
  "columns": 8,
  "spacing": 0,
  "margin": 0,
  "animations": [
    { "name": "walk_s",  "loop": true, "frameRange": { "start": 0,  "end": 7,  "frameDuration": 0.08 } },
    { "name": "walk_se", "loop": true, "frameRange": { "start": 8,  "end": 15, "frameDuration": 0.08 } },
    { "name": "walk_e",  "loop": true, "frameRange": { "start": 16, "end": 23, "frameDuration": 0.08 } },
    { "name": "walk_ne", "loop": true, "frameRange": { "start": 24, "end": 31, "frameDuration": 0.08 } },
    { "name": "walk_n",  "loop": true, "frameRange": { "start": 32, "end": 39, "frameDuration": 0.08 } },
    { "name": "walk_nw", "loop": true, "frameRange": { "start": 40, "end": 47, "frameDuration": 0.08 } },
    { "name": "walk_w",  "loop": true, "frameRange": { "start": 48, "end": 55, "frameDuration": 0.08 } },
    { "name": "walk_sw", "loop": true, "frameRange": { "start": 56, "end": 63, "frameDuration": 0.08 } }
  ]
}
```

### C++ Control Code

```cpp
#include "ECS_Systems_Animation.h"
#include <cmath>

void UpdatePlayerAnimation(ECS_Entity player, Vector velocity)
{
    if (velocity.Length() < 0.1f)
        return;  // Not moving, stay in current animation
    
    // Calculate direction angle
    float angle = std::atan2(velocity.y, velocity.x);
    float degrees = angle * 180.0f / M_PI;
    
    // Normalize to 0-360
    if (degrees < 0) degrees += 360.0f;
    
    // Map angle to 8 directions
    std::string direction;
    if (degrees >= 337.5f || degrees < 22.5f)
        direction = "walk_e";
    else if (degrees >= 22.5f && degrees < 67.5f)
        direction = "walk_ne";
    else if (degrees >= 67.5f && degrees < 112.5f)
        direction = "walk_n";
    else if (degrees >= 112.5f && degrees < 157.5f)
        direction = "walk_nw";
    else if (degrees >= 157.5f && degrees < 202.5f)
        direction = "walk_w";
    else if (degrees >= 202.5f && degrees < 247.5f)
        direction = "walk_sw";
    else if (degrees >= 247.5f && degrees < 292.5f)
        direction = "walk_s";
    else
        direction = "walk_se";
    
    AnimationSystem::Get().PlayAnimation(player, direction, false);
}
```

### Expected Behavior
- Character walks smoothly in any of 8 directions
- Animation changes based on movement direction
- Seamless transitions between directions

---

## Example 3: Attack Combo Chain

### Use Case
Fighting game-style combo system: punch1 → punch2 → punch3 → kick → idle

### Animation Bank JSON

```json
{
  "id": "fighter",
  "spritesheetPath": "Gamedata/Sprites/Fighter/attacks.png",
  "frameWidth": 96,
  "frameHeight": 96,
  "columns": 8,
  "animations": [
    {
      "name": "idle",
      "loop": true,
      "frameRange": { "start": 0, "end": 5, "frameDuration": 0.12 }
    },
    {
      "name": "punch1",
      "loop": false,
      "nextAnimation": "idle",
      "frameRange": { "start": 8, "end": 11, "frameDuration": 0.08 }
    },
    {
      "name": "punch2",
      "loop": false,
      "nextAnimation": "idle",
      "frameRange": { "start": 12, "end": 15, "frameDuration": 0.08 }
    },
    {
      "name": "punch3",
      "loop": false,
      "nextAnimation": "idle",
      "frameRange": { "start": 16, "end": 19, "frameDuration": 0.08 }
    },
    {
      "name": "kick",
      "loop": false,
      "nextAnimation": "idle",
      "frameRange": { "start": 20, "end": 25, "frameDuration": 0.06 }
    }
  ]
}
```

### Animation Graph JSON

```json
{
  "id": "fighter_fsm",
  "defaultState": "idle",
  "states": [
    {
      "name": "idle",
      "animation": "idle",
      "transitions": [
        { "to": "punch1" }
      ]
    },
    {
      "name": "punch1",
      "animation": "punch1",
      "transitions": [
        { "to": "punch2" },
        { "to": "idle" }
      ]
    },
    {
      "name": "punch2",
      "animation": "punch2",
      "transitions": [
        { "to": "punch3" },
        { "to": "idle" }
      ]
    },
    {
      "name": "punch3",
      "animation": "punch3",
      "transitions": [
        { "to": "kick" },
        { "to": "idle" }
      ]
    },
    {
      "name": "kick",
      "animation": "kick",
      "transitions": [
        { "to": "idle" }
      ]
    }
  ]
}
```

### C++ Control Code

```cpp
class ComboSystem
{
    float m_comboWindow = 0.5f;  // 0.5 seconds to continue combo
    float m_lastAttackTime = 0.0f;
    
public:
    void HandleAttackInput(ECS_Entity fighter)
    {
        auto& anim = World::Get().GetComponent<VisualAnimation_data>(fighter);
        float currentTime = GameEngine::GetGameTime();
        
        bool inComboWindow = (currentTime - m_lastAttackTime) < m_comboWindow;
        
        if (InputManager::Get().IsActionJustPressed("attack"))
        {
            if (anim.currentStateName == "idle" || !inComboWindow)
            {
                // Start new combo
                AnimationSystem::Get().TransitionToState(fighter, "punch1");
            }
            else if (anim.currentStateName == "punch1" && inComboWindow)
            {
                AnimationSystem::Get().TransitionToState(fighter, "punch2");
            }
            else if (anim.currentStateName == "punch2" && inComboWindow)
            {
                AnimationSystem::Get().TransitionToState(fighter, "punch3");
            }
            else if (anim.currentStateName == "punch3" && inComboWindow)
            {
                AnimationSystem::Get().TransitionToState(fighter, "kick");
            }
            
            m_lastAttackTime = currentTime;
        }
        
        // Auto-return to idle if combo times out
        if (!inComboWindow && anim.currentStateName != "idle")
        {
            if (anim.isAnimationComplete)
            {
                AnimationSystem::Get().TransitionToState(fighter, "idle");
            }
        }
    }
};
```

### Expected Behavior
- Player presses attack repeatedly to chain combo
- Window of 0.5s to continue combo, else returns to idle
- Full combo: punch1 → punch2 → punch3 → kick → idle
- Timing-based gameplay (rhythm-action feel)

---

## Example 4: Conditional Transitions Based on Velocity

### Use Case
Character automatically switches between idle/walk/run based on movement speed.

### Animation Graph JSON

```json
{
  "id": "auto_movement_fsm",
  "defaultState": "idle",
  "states": [
    {
      "name": "idle",
      "animation": "idle",
      "transitions": [
        { "to": "walk", "condition": "velocity > 0" },
        { "to": "run", "condition": "velocity > 200" }
      ]
    },
    {
      "name": "walk",
      "animation": "walk",
      "transitions": [
        { "to": "idle", "condition": "velocity == 0" },
        { "to": "run", "condition": "velocity > 200" }
      ]
    },
    {
      "name": "run",
      "animation": "run",
      "transitions": [
        { "to": "walk", "condition": "velocity <= 200 && velocity > 0" },
        { "to": "idle", "condition": "velocity == 0" }
      ]
    }
  ]
}
```

### C++ Control Code

```cpp
void AutoUpdateMovementAnimation(ECS_Entity entity)
{
    auto& physics = World::Get().GetComponent<Physics_data>(entity);
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    
    float speed = physics.velocity.Length();
    
    // Hysteresis thresholds to prevent oscillation
    const float WALK_THRESHOLD = 10.0f;
    const float RUN_THRESHOLD = 200.0f;
    
    if (speed < WALK_THRESHOLD)
    {
        if (anim.currentStateName != "idle")
        {
            AnimationSystem::Get().TransitionToState(entity, "idle");
        }
    }
    else if (speed < RUN_THRESHOLD)
    {
        if (anim.currentStateName != "walk")
        {
            AnimationSystem::Get().TransitionToState(entity, "walk");
        }
    }
    else
    {
        if (anim.currentStateName != "run")
        {
            AnimationSystem::Get().TransitionToState(entity, "run");
        }
    }
}
```

### Expected Behavior
- Character idles when stationary
- Walks when moving slowly
- Runs when moving fast
- Smooth automatic transitions based on physics

---

## Example 5: Non-Looping Animations (Jump, Death)

### Use Case
One-shot animations that play once and either stop or transition to another state.

### Animation Bank JSON

```json
{
  "id": "platformer_character",
  "spritesheetPath": "Gamedata/Sprites/Platformer/hero.png",
  "frameWidth": 64,
  "frameHeight": 64,
  "columns": 10,
  "animations": [
    {
      "name": "idle",
      "loop": true,
      "frameRange": { "start": 0, "end": 5, "frameDuration": 0.1 }
    },
    {
      "name": "jump",
      "loop": false,
      "nextAnimation": "fall",
      "frameRange": { "start": 10, "end": 12, "frameDuration": 0.1 }
    },
    {
      "name": "fall",
      "loop": true,
      "frameRange": { "start": 13, "end": 14, "frameDuration": 0.15 }
    },
    {
      "name": "land",
      "loop": false,
      "nextAnimation": "idle",
      "frameRange": { "start": 15, "end": 16, "frameDuration": 0.08 }
    },
    {
      "name": "death",
      "loop": false,
      "frameRange": { "start": 20, "end": 29, "frameDuration": 0.1 }
    }
  ]
}
```

### C++ Control Code

```cpp
void HandleJumpPhysics(ECS_Entity player)
{
    auto& physics = World::Get().GetComponent<Physics_data>(player);
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(player);
    
    bool isGrounded = CheckGrounded(player);
    
    // Jump input
    if (InputManager::Get().IsActionJustPressed("jump") && isGrounded)
    {
        physics.velocity.y = -500.0f;  // Jump velocity
        AnimationSystem::Get().PlayAnimation(player, "jump", true);
    }
    
    // Falling detection
    if (!isGrounded && physics.velocity.y > 0)
    {
        if (anim.currentAnimName != "fall")
        {
            AnimationSystem::Get().PlayAnimation(player, "fall", false);
        }
    }
    
    // Landing detection
    if (isGrounded && anim.currentAnimName == "fall")
    {
        AnimationSystem::Get().PlayAnimation(player, "land", true);
        // "land" animation auto-transitions to "idle"
    }
}

void HandleDeath(ECS_Entity entity)
{
    auto& health = World::Get().GetComponent<Health_data>(entity);
    
    if (health.current <= 0)
    {
        AnimationSystem::Get().PlayAnimation(entity, "death", true);
        
        // Death animation plays once and stops on last frame
        // Entity becomes inactive after animation completes
    }
}
```

### Expected Behavior
- **Jump**: Plays once, auto-transitions to fall
- **Fall**: Loops while in air
- **Land**: Plays once, auto-transitions to idle
- **Death**: Plays once, stops on last frame (corpse)

---

## Example 6: Animation Events (Footstep Sounds)

### Use Case
Trigger sound effects or gameplay events on specific animation frames.

### Animation Bank JSON (Explicit Frames with Events)

```json
{
  "id": "character_with_sounds",
  "spritesheetPath": "Gamedata/Sprites/Characters/hero.png",
  "frameWidth": 64,
  "frameHeight": 64,
  "columns": 8,
  "animations": [
    {
      "name": "walk",
      "loop": true,
      "speed": 1.0,
      "frames": [
        { "srcRect": { "x": 0, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
        { "srcRect": { "x": 64, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
        { "srcRect": { "x": 128, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": ["footstep_left"] },
        { "srcRect": { "x": 192, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
        { "srcRect": { "x": 256, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
        { "srcRect": { "x": 320, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] },
        { "srcRect": { "x": 384, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": ["footstep_right"] },
        { "srcRect": { "x": 448, "y": 64, "w": 64, "h": 64 }, "duration": 0.1, "events": [] }
      ]
    },
    {
      "name": "attack",
      "loop": false,
      "nextAnimation": "idle",
      "frames": [
        { "srcRect": { "x": 0, "y": 128, "w": 64, "h": 64 }, "duration": 0.15, "events": [] },
        { "srcRect": { "x": 64, "y": 128, "w": 64, "h": 64 }, "duration": 0.05, "events": ["sword_swing"] },
        { "srcRect": { "x": 128, "y": 128, "w": 64, "h": 64 }, "duration": 0.05, "events": ["hit_impact"] },
        { "srcRect": { "x": 192, "y": 128, "w": 64, "h": 64 }, "duration": 0.15, "events": [] }
      ]
    }
  ]
}
```

### C++ Event Handling

```cpp
class AnimationEventHandler
{
public:
    void ProcessAnimationEvents(ECS_Entity entity)
    {
        auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
        
        // Get events for current frame
        const auto& currentFrame = anim.currentSequence->frames[anim.currentFrame];
        
        for (const std::string& event : currentFrame.events)
        {
            HandleEvent(entity, event);
        }
    }
    
private:
    void HandleEvent(ECS_Entity entity, const std::string& eventName)
    {
        if (eventName == "footstep_left" || eventName == "footstep_right")
        {
            PlaySound("Sounds/footstep.wav", 0.5f);
            CreateParticle(entity, "dust_puff");
        }
        else if (eventName == "sword_swing")
        {
            PlaySound("Sounds/sword_whoosh.wav", 0.7f);
        }
        else if (eventName == "hit_impact")
        {
            PlaySound("Sounds/sword_hit.wav", 0.8f);
            CheckHitboxCollision(entity);
        }
    }
};
```

### AnimationSystem Integration

```cpp
// In AnimationSystem::Process()
void AnimationSystem::Process(float deltaTime)
{
    for (ECS_Entity entity : m_entities)
    {
        auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
        
        int previousFrame = anim.currentFrame;
        
        // Update animation frame
        UpdateAnimationFrame(entity, deltaTime);
        
        // Trigger events on frame change
        if (anim.currentFrame != previousFrame)
        {
            TriggerFrameEvents(entity, anim.currentFrame);
        }
    }
}

void AnimationSystem::TriggerFrameEvents(ECS_Entity entity, int frameIndex)
{
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(entity);
    
    if (!anim.currentSequence || frameIndex >= anim.currentSequence->frames.size())
        return;
    
    const auto& frame = anim.currentSequence->frames[frameIndex];
    
    for (const std::string& event : frame.events)
    {
        EventManager::Get().TriggerEvent("animation_event", entity, event);
    }
}
```

### Expected Behavior
- Footstep sounds play on frames 2 and 6 of walk cycle
- Sword swoosh plays on attack frame 1
- Hit impact plays on attack frame 2
- Events synchronized perfectly with animation

---

## Example 7: Directional Sprites with FlipX

### Use Case
Reduce spritesheet memory by flipping sprites horizontally for left/right directions.

### Animation Bank JSON

```json
{
  "id": "side_scroller_char",
  "spritesheetPath": "Gamedata/Sprites/Characters/hero_right.png",
  "frameWidth": 64,
  "frameHeight": 64,
  "columns": 8,
  "animations": [
    {
      "name": "idle_right",
      "loop": true,
      "frameRange": { "start": 0, "end": 5, "frameDuration": 0.12 }
    },
    {
      "name": "walk_right",
      "loop": true,
      "frameRange": { "start": 8, "end": 15, "frameDuration": 0.08 }
    },
    {
      "name": "run_right",
      "loop": true,
      "frameRange": { "start": 16, "end": 23, "frameDuration": 0.06 }
    }
  ]
}
```

### C++ Control Code with FlipX

```cpp
void UpdateSideScrollerAnimation(ECS_Entity player, Vector velocity)
{
    auto& sprite = World::Get().GetComponent<VisualSprite_data>(player);
    auto& anim = World::Get().GetComponent<VisualAnimation_data>(player);
    
    float speed = std::abs(velocity.x);
    bool movingRight = velocity.x > 0;
    
    // Determine animation
    std::string animName;
    if (speed < 10.0f)
        animName = "idle_right";
    else if (speed < 200.0f)
        animName = "walk_right";
    else
        animName = "run_right";
    
    // Flip sprite for left movement
    sprite.flipX = !movingRight;
    
    // Play animation
    if (anim.currentAnimName != animName)
    {
        AnimationSystem::Get().PlayAnimation(player, animName, false);
    }
}
```

### Expected Behavior
- Character uses only right-facing sprites
- Left movement flips sprite horizontally
- Saves 50% of spritesheet memory
- No visual difference from full directional sprites

---

## Example 8: Variable Frame Durations (Attack Anticipation)

### Use Case
Attack animation with slow wind-up, fast strike, and slow recovery for game feel.

### Animation Bank JSON

```json
{
  "id": "heavy_warrior",
  "spritesheetPath": "Gamedata/Sprites/Warrior/attacks.png",
  "frameWidth": 128,
  "frameHeight": 128,
  "columns": 6,
  "animations": [
    {
      "name": "heavy_attack",
      "loop": false,
      "nextAnimation": "idle",
      "frames": [
        { "srcRect": { "x": 0, "y": 0, "w": 128, "h": 128 }, "duration": 0.3, "events": [] },      // Wind-up
        { "srcRect": { "x": 128, "y": 0, "w": 128, "h": 128 }, "duration": 0.25, "events": [] },   // Charge
        { "srcRect": { "x": 256, "y": 0, "w": 128, "h": 128 }, "duration": 0.05, "events": ["swing"] },   // Strike!
        { "srcRect": { "x": 384, "y": 0, "w": 128, "h": 128 }, "duration": 0.05, "events": ["impact"] },  // Impact
        { "srcRect": { "x": 512, "y": 0, "w": 128, "h": 128 }, "duration": 0.2, "events": [] },    // Recovery
        { "srcRect": { "x": 640, "y": 0, "w": 128, "h": 128 }, "duration": 0.15, "events": [] }    // Return
      ]
    }
  ]
}
```

### Expected Behavior
- Total animation time: 1.0 second
- Wind-up: 0.55s (frames 0-1) - telegraphs attack
- Strike: 0.1s (frames 2-3) - very fast!
- Recovery: 0.35s (frames 4-5) - vulnerable window
- Creates satisfying weight and impact

---

## Summary Table

| Example | Technique | Complexity | Use Case |
|---------|-----------|------------|----------|
| 1. Simple Idle | Basic loop | ⭐ | Static NPCs |
| 2. 8-Directional Walk | Multiple animations | ⭐⭐ | Top-down games |
| 3. Attack Combo | FSM transitions | ⭐⭐⭐ | Fighting games |
| 4. Velocity-Based | Conditional switching | ⭐⭐ | Platformers |
| 5. Jump/Death | Non-looping | ⭐⭐ | Platformers |
| 6. Sound Events | Frame events | ⭐⭐⭐ | Polished games |
| 7. FlipX | Memory optimization | ⭐⭐ | Side-scrollers |
| 8. Variable Timing | Advanced feel | ⭐⭐⭐ | Action games |

## See Also

- [Quick Start Guide](animation-system-quick-start.md) - Get started with animations
- [Animation Banks Reference](animation-banks.md) - Complete JSON reference
- [Animation Graphs Reference](animation-graphs.md) - FSM documentation
- [Troubleshooting Guide](troubleshooting.md) - Fix common issues
