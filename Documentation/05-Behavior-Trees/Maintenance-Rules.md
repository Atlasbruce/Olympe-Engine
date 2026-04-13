# Behavior Tree Maintenance Rules

Best practices for keeping BT graphs maintainable.

## Rule 1: One Responsibility Per Tree

Each `.bt` file should implement **one high-level behavior**:
- ✅ `combat.bt` – Combat behavior only
- ✅ `patrol.bt` – Patrol loop only
- ❌ `guard_everything.bt` – Everything in one file

## Rule 2: Prefer SubGraphs for Reuse

If the same sequence appears in 3+ trees, extract it to a subgraph:

```
my_combat.bt
├── UseSubGraph("ranged_attack.bt")
└── UseSubGraph("flee.bt")
```

## Rule 3: Limit Depth

Keep BT depth ≤ 6 levels. Deeper trees are hard to debug.

## Rule 4: Name Blackboard Keys Consistently

Use a prefix convention:
- `player_*` – Player-related state
- `self_*` – Entity's own state
- `world_*` – World state (from GlobalBlackboard)

## Rule 5: Use Cooldowns for Performance

Wrap expensive conditions in Cooldown decorators:

```
Cooldown(0.5s)
└── HasLineOfSight   ← expensive raycast, checked only every 0.5s
```

## Rule 6: Document Complex Branches

Add a Comment node at the start of non-obvious sequences:

```
Comment("Combat: only engage if health > 50% and player visible")
Sequence
├── CheckBlackboard(health > 50)
├── HasLineOfSight
└── Attack
```

## Rule 7: Test in Isolation

Test each BT recipe in a minimal test scene before integrating.
