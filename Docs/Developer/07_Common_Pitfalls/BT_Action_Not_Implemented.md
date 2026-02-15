# ⚠️ Common Pitfall: BT Action Not Implemented

**Problem:** Behavior tree references an action that doesn't exist in the engine

---

## 🐛 Symptoms

### Console Output
```
[BehaviorTreeManager] SUCCESS: Loaded 'NPCWanderTree' with 4 nodes
[BT] Entity 1770972786312977800 | Node: Wait Random Time (2-6s) | Status: FAILURE
[BT] Entity 1770972786312977800 | Node: Choose Random Navigable Point | Status: FAILURE
[BT] Entity 1770972786312977800 | Node: Request Pathfinding | Status: FAILURE
```

### Behavior
- NPCs don't move
- Behavior tree always returns FAILURE
- No errors during BT loading
- Tree executes but actions fail immediately

---

## 🔍 Root Cause

**The action is defined in the JSON but NOT implemented in the engine code.**

### How It Happens

1. **Designer creates BT in JSON:**
```json
{
  "actionType": "WaitRandomTime",
  "parameters": {
    "param1": 2.0,
    "param2": 6.0
  }
}
```

2. **BT Manager parses JSON successfully** ✅
   - Action name read from JSON
   - Parameters extracted
   - Node created

3. **BUT: Action mapping missing** ❌
```cpp
// In BehaviorTree.cpp
if (actStr == "SetMoveGoalToTarget")
    node.actionType = BTActionType::SetMoveGoalToTarget;
else if (actStr == "MoveToGoal")
    node.actionType = BTActionType::MoveToGoal;
// Missing: "WaitRandomTime" mapping!
```

4. **Result: Default action type (0)** used, which returns FAILURE

---

## ✅ Solution

Implement the missing action in **4 steps**:

### Step 1: Add to BTActionType Enum

**File:** `Source/AI/BehaviorTree.h`

```cpp
enum class BTActionType : uint8_t
{
    // ... existing actions ...
    MoveToGoal,
    AttackIfClose,
    
    // NEW: Add your action here
    WaitRandomTime,                  // ← Add this
    ChooseRandomNavigablePoint,
    RequestPathfinding,
    FollowPath
};
```

### Step 2: Map Action Name

**File:** `Source/AI/BehaviorTree.cpp` (in LoadTree function)

```cpp
// Parse action type
if (node.type == BTNodeType::Action && nodeJson.contains("actionType"))
{
    std::string actStr = JsonHelper::GetString(nodeJson, "actionType", "");
    
    // ... existing mappings ...
    
    // NEW: Add your action mapping
    else if (actStr == "WaitRandomTime")
        node.actionType = BTActionType::WaitRandomTime;
    else if (actStr == "ChooseRandomNavigablePoint")
        node.actionType = BTActionType::ChooseRandomNavigablePoint;
    else if (actStr == "RequestPathfinding")
        node.actionType = BTActionType::RequestPathfinding;
    else if (actStr == "FollowPath")
        node.actionType = BTActionType::FollowPath;
}
```

### Step 3: Implement Action Logic

**File:** `Source/AI/BehaviorTree.cpp` (in ExecuteBTAction function)

```cpp
BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, 
                         EntityID entity, AIBlackboard_data& blackboard)
{
    switch (actionType)
    {
        // ... existing cases ...
        
        // NEW: Implement your action
        case BTActionType::WaitRandomTime:
        {
            // Initialize timer on first call
            if (blackboard.wanderTargetWaitTime == 0.0f)
            {
                float minWait = (param1 > 0.0f) ? param1 : 2.0f;
                float maxWait = (param2 > 0.0f) ? param2 : 6.0f;
                
                float randomFactor = (float)rand() / RAND_MAX;
                blackboard.wanderTargetWaitTime = minWait + randomFactor * (maxWait - minWait);
                blackboard.wanderWaitTimer = 0.0f;
            }
            
            // Increment timer
            blackboard.wanderWaitTimer += GameEngine::fDt;
            
            // Check if complete
            if (blackboard.wanderWaitTimer >= blackboard.wanderTargetWaitTime)
            {
                blackboard.wanderTargetWaitTime = 0.0f;
                blackboard.wanderWaitTimer = 0.0f;
                return BTStatus::Success;
            }
            
            return BTStatus::Running;
        }
        
        // ... more new actions ...
    }
    
    return BTStatus::Failure;
}
```

### Step 4: Add Required Blackboard Fields

**File:** `Source/ECS_Components_AI.h`

```cpp
struct AIBlackboard_data
{
    // ... existing fields ...
    
    // NEW: Add fields needed by your action
    float wanderWaitTimer = 0.0f;
    float wanderTargetWaitTime = 0.0f;
    Vector wanderDestination;
    bool hasWanderDestination = false;
};
AUTO_REGISTER_COMPONENT(AIBlackboard_data);  // Don't forget this!
```

---

## 🧪 Verification

### Test Your Implementation

1. **Build the engine** (ensure no compile errors)

2. **Run the game** with debug logging:
```cpp
SYSTEM_LOG << "[BT] Entity " << entity 
           << " | Node: " << node->name 
           << " | Status: " << statusName << "\n";
```

3. **Expected output:**
```
[BT] Entity 1234 | Node: Wait Random Time (2-6s) | Status: RUNNING
[BT] Entity 1234 | Node: Wait Random Time (2-6s) | Status: RUNNING
[BT] Entity 1234 | Node: Wait Random Time (2-6s) | Status: SUCCESS
[BT] Entity 1234 | Node: Choose Random Navigable Point | Status: SUCCESS
[BT] Entity 1234 | Node: Request Pathfinding | Status: SUCCESS
[BT] Entity 1234 | Node: Follow Path | Status: RUNNING
```

4. **Verify NPC behavior:**
- NPCs wait random time (2-6 seconds)
- NPCs choose random destination
- NPCs path to destination
- NPCs repeat cycle

---

## 📋 Action Implementation Checklist

When adding ANY new BT action:

- [ ] Add enum value to `BTActionType` (BehaviorTree.h)
- [ ] Add string-to-enum mapping (BehaviorTree.cpp)
- [ ] Implement action logic in `ExecuteBTAction()` (BehaviorTree.cpp)
- [ ] Add required blackboard fields (ECS_Components_AI.h)
- [ ] Handle parameters correctly (param1, param2)
- [ ] Return appropriate status (Success/Running/Failure)
- [ ] Add debug logging
- [ ] Test with actual BT JSON
- [ ] Verify NPC behavior in-game

---

## 🚨 Related Pitfalls

### Pitfall 1: Action Parses But Wrong Enum Value

**Symptom:** Action name recognized but wrong behavior

**Cause:** Typo in string comparison
```cpp
// BAD: Case-sensitive
if (actStr == "waitrandomtime")  // JSON has "WaitRandomTime"
```

**Fix:** Match exact case from JSON
```cpp
if (actStr == "WaitRandomTime")  // Exact match
```

---

### Pitfall 2: Parameters Not Parsed

**Symptom:** Action runs but uses default values

**Cause:** Missing parameter extraction
```cpp
// BAD: Not reading parameters
case BTActionType::WaitRandomTime:
{
    float minWait = 2.0f;  // Hardcoded!
    float maxWait = 6.0f;  // Hardcoded!
}
```

**Fix:** Use param1, param2 from function args
```cpp
float minWait = (param1 > 0.0f) ? param1 : 2.0f;
float maxWait = (param2 > 0.0f) ? param2 : 6.0f;
```

---

### Pitfall 3: Missing Blackboard Fields

**Symptom:** Crashes when accessing blackboard

**Cause:** Accessing field that doesn't exist
```cpp
// CRASH: Field not defined in AIBlackboard_data
blackboard.wanderWaitTimer += GameEngine::fDt;
```

**Fix:** Add field to struct
```cpp
struct AIBlackboard_data
{
    float wanderWaitTimer = 0.0f;  // Add field
};
```

---

### Pitfall 4: Forgetting to Reset State

**Symptom:** Action works once then breaks

**Cause:** Not resetting blackboard state
```cpp
// BAD: Timer never resets
blackboard.wanderWaitTimer += GameEngine::fDt;
if (blackboard.wanderWaitTimer >= target)
{
    return BTStatus::Success;  // Never resets timer!
}
```

**Fix:** Reset on completion
```cpp
if (blackboard.wanderWaitTimer >= target)
{
    blackboard.wanderWaitTimer = 0.0f;      // Reset
    blackboard.wanderTargetWaitTime = 0.0f; // Reset
    return BTStatus::Success;
}
```

---

### Pitfall 5: Wrong Return Status

**Symptom:** Action completes but tree doesn't progress

**Cause:** Always returning Running or Failure
```cpp
// BAD: Never returns Success
case BTActionType::WaitRandomTime:
{
    // ... wait logic ...
    return BTStatus::Running;  // Never completes!
}
```

**Fix:** Return Success when done
```cpp
if (blackboard.wanderWaitTimer >= target)
{
    // ... reset state ...
    return BTStatus::Success;  // ✅ Completes
}
return BTStatus::Running;  // Still waiting
```

---

## 🔧 Debug Tools

### Enable Verbose BT Logging

```cpp
// In BehaviorTreeSystem::Process()
#define DEBUG_BT_EXECUTION 1

#if DEBUG_BT_EXECUTION
SYSTEM_LOG << "[BT] Entity " << entity 
           << " | Node: " << node->name 
           << " | Action: " << (int)node->actionType
           << " | Status: " << statusName 
           << " | Params: (" << node->actionParam1 << ", " << node->actionParam2 << ")"
           << "\n";
#endif
```

### Visualize Blackboard State

```cpp
void DebugBlackboard(EntityID entity)
{
    AIBlackboard_data& bb = World::Get().GetComponent<AIBlackboard_data>(entity);
    
    SYSTEM_LOG << "=== Blackboard [" << entity << "] ===\n";
    SYSTEM_LOG << "  wanderWaitTimer: " << bb.wanderWaitTimer << "\n";
    SYSTEM_LOG << "  wanderTargetWaitTime: " << bb.wanderTargetWaitTime << "\n";
    SYSTEM_LOG << "  hasWanderDestination: " << bb.hasWanderDestination << "\n";
    SYSTEM_LOG << "  wanderDestination: (" << bb.wanderDestination.x << ", " 
               << bb.wanderDestination.y << ")\n";
    SYSTEM_LOG << "==============================\n";
}
```

---

## 📚 See Also

- [Behavior Trees Guide](../04_Behavior_Trees/Creating_BT_Actions_Guide.md)
- [AI Systems Reference](../05_AI_Systems/_QuickRef_AI_Pipeline.md)
- [Component Registration](Missing_Component_Registration.md)

---

**💡 Pro Tip:** Always implement AND test one action at a time. Don't batch-implement multiple actions without testing each one!
