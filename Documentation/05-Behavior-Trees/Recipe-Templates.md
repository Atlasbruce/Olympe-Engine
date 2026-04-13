# Behavior Tree Recipes

Ready-to-use BT patterns for common AI behaviors.

## 1. Patrol + Alert

```
Selector
├── Sequence (if alerted)
│   ├── CheckBlackboard(isAlerted == true)
│   ├── MoveToTarget(playerPos)
│   └── Attack
└── Sequence (patrol)
    ├── SetBlackboard(isAlerted = false)
    ├── MoveToWaypoint(A)
    ├── Wait(1.5s)
    ├── MoveToWaypoint(B)
    └── Wait(1.5s)
```

## 2. Chase and Attack

```
Sequence
├── IsInRange(detectRadius)
├── SetBlackboard(target = playerPos)
├── Selector
│   ├── IsInRange(attackRadius) → Attack
│   └── MoveToTarget
└── PlayAnimation("attack")
```

## 3. Low-Health Flee

```
Selector
├── Sequence (flee if low health)
│   ├── CheckBlackboard(health < 30)
│   ├── SetBlackboard(state = fleeing)
│   └── MoveAwayFromTarget
└── NormalBehavior (subgraph)
```

## 4. Guard Post

```
Selector
├── Sequence (return to post if too far)
│   ├── CheckBlackboard(distanceFromPost > 200)
│   └── MoveToPost
└── Idle
    ├── Wait(random 2-5s)
    └── LookAround
```

## 5. Loot Collection

```
Sequence
├── FindNearestItem
├── SetBlackboard(targetItem = found)
├── MoveToTarget(targetItem)
├── PickupItem
└── SetBlackboard(inventory += 1)
```
