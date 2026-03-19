# ✅ QUALITY STANDARDS — UNIFIED & CONSOLIDATED
**Last Updated:** 2026-03-19 11:45:00 UTC  
**Version:** 2.0 (Consolidated from 5 source files)  
**Status:** ✅ COMPLETE — Baseline for all agents

---

## TABLE OF CONTENTS

1. [Philosophy](#philosophy)
2. [C++ Code Standards](#c-code-standards)
3. [Architecture Standards](#architecture-standards)
4. [Testing Requirements](#testing-requirements)
5. [Documentation Standards](#documentation-standards)
6. [Quality Gates](#quality-gates)
7. [Code Review Checklist](#code-review-checklist)
8. [Performance Benchmarks](#performance-benchmarks)

---

## PHILOSOPHY

**"Clean code that scales. Simple to understand. Hard to break. Easy to extend."**

### Core Principles

1. **Clarity First** — Code reads like documentation
2. **Modularity** — Each component does one thing well
3. **Testability** — All logic is testable in isolation
4. **Performance** — Optimize for frames, not files
5. **Maintainability** — Future devs will thank you

---

## C++ CODE STANDARDS

### Language Version
- ✅ **C++14 ONLY** — No C++17, C++20, or later features
- Reason: Cross-platform compatibility, MSVC support
- Allowed: `auto`, lambdas, `make_unique`, `nullptr`
- NOT allowed: `std::optional`, `std::variant`, structured bindings

### Naming Conventions

**Classes & Types:**
```cpp
// ✅ CORRECT
class EntityManager { };
struct TransformComponent { };
enum class SystemPriority { };

// ❌ WRONG
class entity_manager { };
class EManager { };
typedef int MyInt;
```

**Functions & Methods:**
```cpp
// ✅ CORRECT
void UpdatePhysics(float deltaTime);
bool IsValidEntity(EntityID id);
float GetMass() const;

// ❌ WRONG
void update_physics(float dt);
void UPDATEPHYSICS();
bool valid_entity(int id);
```

**Variables & Members:**
```cpp
// ✅ CORRECT
int entityCount;
float maxVelocity;
std::vector<Component> components;
static constexpr int MAX_ENTITIES = 10000;

// ❌ WRONG
int entity_count;
float mMaxVelocity;
int m_entity_count;
#define MAX_ENT 10000
```

**Constants:**
```cpp
// ✅ CORRECT
static constexpr float GRAVITY = 9.81f;
static constexpr int MAX_LAYERS = 32;
enum class Color : uint8_t { Red, Green, Blue };

// ❌ WRONG
const float gravity = 9.81f; // should be static constexpr
#define GRAVITY 9.81f
static float GRAVITY = 9.81f;
```

### Header Guidelines

**Every public header MUST include:**

```cpp
#pragma once  // NOT #ifndef guards

#include <vector>
#include <memory>
#include "Utils/Math.h"
#include "Core/Entity.h"

// Forward declarations (no includes if possible)
class World;
class System;

namespace olympe {
  namespace core {
    
// All code here

  } // namespace core
} // namespace olympe
```

**Rules:**
- ✅ Use `#pragma once` (simpler, faster)
- ✅ Include order: STL → project headers → forward decls
- ✅ Use namespaces (olympe::core, olympe::rendering, etc.)
- ✅ Group related includes together
- ❌ Never put implementation in headers (except templates)
- ❌ Avoid circular dependencies (use forward decls)

### Functions & Methods

**Length:**
```cpp
// ✅ OK: ~30-50 lines (single responsibility)
void UpdatePhysics(World& world, float dt) {
  auto& entities = world.GetEntities();
  for (auto& entity : entities) {
    if (!entity.HasComponent<RigidBodyComponent>()) continue;
    auto& rb = entity.GetComponent<RigidBodyComponent>();
    // ... physics simulation ...
  }
}

// ❌ TOO LONG: >100 lines (break into smaller functions)
void UpdateEverything(World& world, float dt) {
  // Physics
  // Rendering
  // Audio
  // AI
  // ... 200 lines total ...
}
```

**Pure Functions Preferred:**
```cpp
// ✅ GOOD: No side effects
bool IsColliding(const AABB& a, const AABB& b) {
  return !(a.min.x > b.max.x || a.max.x < b.min.x ||
           a.min.y > b.max.y || a.max.y < b.min.y);
}

// ⚠️ ACCEPTABLE: Only side effect is output parameter
void GetIntersectionRect(const AABB& a, const AABB& b, AABB& out) {
  out.min.x = std::max(a.min.x, b.min.x);
  out.min.y = std::max(a.min.y, b.min.y);
  out.max.x = std::min(a.max.x, b.max.x);
  out.max.y = std::min(a.max.y, b.max.y);
}

// ❌ BAD: Multiple side effects
void ProcessEntity(Entity& e) {
  gWorld->RemoveEntity(e.id);  // Side effect 1
  gLogger.Log("Removed entity");  // Side effect 2
  e.data = nullptr;  // Side effect 3
}
```

**Const Correctness:**
```cpp
// ✅ CORRECT
class Entity {
public:
  vec2 GetPosition() const { return position; }
  void SetPosition(const vec2& pos) { position = pos; }
  
  Component& GetComponent() { return component; }
  const Component& GetComponent() const { return component; }
};

// ❌ WRONG
class Entity {
public:
  vec2 GetPosition() { return position; }  // Not const!
  Component GetComponent() const;  // Should return by ref
};
```

### Classes & Objects

**Size & Responsibility:**
```cpp
// ✅ GOOD: Single responsibility
class RigidBody {
  vec2 position, velocity;
  float mass;
public:
  void ApplyForce(const vec2& force);
  void Integrate(float dt);
  vec2 GetVelocity() const;
};

// ❌ TOO BIG: Multiple responsibilities
class PhysicsObject {
  // Transforms
  // Rendering
  // Physics
  // Audio
  // Networking
  // ... 1000 lines ...
};
```

**Initialization:**
```cpp
// ✅ CORRECT: Always initialize
class Component {
  int id = 0;  // In-class initializer
  float value = 1.0f;
  std::vector<int> data;
  
  Component() : id(0), value(1.0f) { }  // Constructor
};

// ❌ WRONG: Uninitialized members
class Component {
  int id;  // What's the default?
  float value;  // Undefined behavior
};
```

**Smart Pointers:**
```cpp
// ✅ PREFERRED
std::unique_ptr<Entity> entity = std::make_unique<Entity>();
std::vector<std::shared_ptr<Component>> components;

// ⚠️ ACCEPTABLE (for performance-critical paths)
Entity* entity = new Entity();  // ... eventually delete

// ❌ NEVER
Entity entity;  // Don't allocate on stack (except small things)
```

### Memory Management

**Stack vs Heap:**
```cpp
// ✅ STACK (small, fixed-size, automatic cleanup)
vec2 position;  // 8 bytes
Transform transform;  // < 64 bytes typically
int buffer[256];  // <= 1 KB

// ✅ HEAP (large, dynamic, manual cleanup or smart ptr)
std::vector<Entity> entities;  // Dynamic array
std::unique_ptr<Texture> texture;  // Texture data
std::map<std::string, Component> registry;  // Dynamic map

// ⚠️ USE OBJECT POOL for frequently created/destroyed objects
ObjectPool<Bullet> bulletPool(1000);  // Pre-allocate 1000
Bullet* bullet = bulletPool.Allocate();
// ... use bullet ...
bulletPool.Free(bullet);
```

**Ownership Model:**
```cpp
// ✅ CLEAR OWNERSHIP
class World {
  std::vector<std::unique_ptr<Entity>> entities;  // World owns entities
public:
  Entity* CreateEntity() {
    entities.push_back(std::make_unique<Entity>());
    return entities.back().get();  // Return non-owning ptr
  }
};

// ❌ AMBIGUOUS OWNERSHIP
class World {
  std::vector<Entity*> entities;  // Who deletes these?
};
```

### Lambdas & Callbacks

```cpp
// ✅ SIMPLE LAMBDAS (1-3 lines)
world.ForEach<SpriteComponent>([](SpriteComponent& s) {
  s.tintColor = Color::Red;
});

// ⚠️ COMPLEX LAMBDAS (extract to function)
world.ForEach<RigidBodyComponent>([&](RigidBodyComponent& rb) {
  // ... 20 lines of physics logic ...
  // Move to: void UpdateRigidBody(RigidBodyComponent& rb)
});

// ✅ CAPTURE CAREFULLY
int multiplier = 2;
auto calc = [multiplier](int x) { return x * multiplier; };  // Copy

World* world = &myWorld;
auto update = [world](float dt) { world->Update(dt); };  // Reference OK (world lives long enough)

// ❌ DANGEROUS: Capturing dangling reference
auto bad = [&myLocalVar]() { return myLocalVar * 2; };  // Undefined if myLocalVar dies
```

### Error Handling

**Assertions for Debug:**
```cpp
// ✅ USE ASSERTIONS (disappear in Release)
assert(entity != nullptr);
assert(mass > 0.0f);
assert(components.size() < MAX_COMPONENTS);

// ✅ FOR EXPECTED CONDITIONS: check + graceful handle
if (!entity.IsValid()) {
  Log(LogLevel::Warning, "Invalid entity");
  return;
}
```

**No Exceptions:**
```cpp
// ❌ DON'T USE EXCEPTIONS (perf overhead)
try {
  int x = std::stoi(str);
} catch (std::exception& e) { }

// ✅ USE RETURN CODES or OPTIONALS
bool TryParseInt(const std::string& str, int& out) {
  try {
    out = std::stoi(str);
    return true;
  } catch (...) {
    return false;
  }
}
```

---

## ARCHITECTURE STANDARDS

### Design Patterns

**Entity Component System:**
- ✅ Entities are lightweight (ID + component references)
- ✅ Components hold pure data
- ✅ Systems operate on components
- ✅ Minimize component coupling

**Dependency Injection:**
```cpp
// ✅ GOOD: Dependencies injected
class PhysicsSystem {
  PhysicsWorld& world;
public:
  PhysicsSystem(PhysicsWorld& w) : world(w) { }
};

// ❌ BAD: Hidden dependency on global
class PhysicsSystem {
  void Update() {
    gWorld->UpdatePhysics();  // Implicit dependency
  }
};
```

**Avoid God Objects:**
```cpp
// ✅ GOOD: Separated concerns
class World { /* Entity management */ };
class Renderer { /* Rendering */ };
class PhysicsWorld { /* Physics */ };

// ❌ BAD: Everything in one class
class GameEngine {
  // Entities, rendering, physics, audio, AI, networking, ...
};
```

---

## TESTING REQUIREMENTS

### Unit Tests (MANDATORY)

**When to write unit tests:**
- ✅ All public functions in new components
- ✅ All math utilities (collisions, transforms)
- ✅ Serialization/deserialization logic
- ✅ State machines, behavior trees

**Test Structure:**
```cpp
// Tests/BlueprintEditor/test_blueprint_compiler.cpp
#include <gtest/gtest.h>
#include "BlueprintEditor/BlueprintCompiler.h"

class BlueprintCompilerTest : public ::testing::Test {
protected:
  BlueprintCompiler compiler;
  Blueprint testBlueprint;
};

TEST_F(BlueprintCompilerTest, CompileSimpleFunction) {
  testBlueprint.AddNode("SetVariable");
  EXPECT_TRUE(compiler.Compile(testBlueprint));
  EXPECT_EQ(compiler.GetErrorCount(), 0);
}

TEST_F(BlueprintCompilerTest, DetectInvalidConnections) {
  testBlueprint.AddNode("BadNode");
  testBlueprint.ConnectBadly();
  EXPECT_FALSE(compiler.Compile(testBlueprint));
  EXPECT_GT(compiler.GetErrorCount(), 0);
}
```

**Test Coverage Target:**
- ✅ Core systems: > 80% coverage
- ✅ Rendering: > 60% coverage
- ✅ Utils/Math: > 90% coverage
- ❌ Never: < 50% coverage for new code

### Integration Tests

```cpp
// Tests/Integration/test_editor_workflow.cpp
TEST(EditorWorkflow, CreateAndSaveBlueprint) {
  // 1. Create blueprint in memory
  // 2. Modify it (add nodes)
  // 3. Save to disk
  // 4. Load from disk
  // 5. Verify contents match
  EXPECT_TRUE(loaded == original);
}
```

### Round-Trip Tests (JSON)

```cpp
// If component is serialized to JSON, verify round-trip
TEST(RoundTrip, SpriteComponentJSON) {
  SpriteComponent original;
  original.layer = 5;
  original.tintColor = Color::Red;
  
  json j = original;  // Serialize
  SpriteComponent restored = j.get<SpriteComponent>();  // Deserialize
  
  EXPECT_EQ(restored.layer, 5);
  EXPECT_EQ(restored.tintColor, Color::Red);
}
```

---

## DOCUMENTATION STANDARDS

### Doxygen Comments (MANDATORY for public API)

```cpp
/// @brief Applies a force to the rigid body.
/// 
/// The force is accumulated and applied during the next physics step.
/// Multiple calls in the same frame sum the forces.
/// 
/// @param force The force vector (in Newtons)
/// @return true if force was applied, false if body is static
/// @see ApplyImpulse, SetVelocity
/// @note This is a frame-delayed operation
bool ApplyForce(const vec2& force);

/// @brief Creates an entity in the world.
/// @return EntityID of the new entity
/// @throw std::runtime_error if max entities reached
EntityID CreateEntity();
```

**Minimum Documentation:**
- ✅ All public classes (1-2 line summary)
- ✅ All public functions (what it does, parameters, return)
- ✅ Tricky code (why, not what)
- ❌ Private implementation details (not needed)
- ❌ Self-explanatory getters/setters

### File Headers

```cpp
///////////////////////////////////////////////////////////////////////////////
/// @file PhysicsSystem.h
/// @brief Physics simulation system (ECS system)
/// 
/// Handles rigid body dynamics, collision detection, and constraint resolution.
/// Updates are performed on a separate task in the TaskGraph for parallelism.
///
/// @author @Atlasbruce
/// @version 1.0
/// @date 2026-03-15
///////////////////////////////////////////////////////////////////////////////
```

### Architecture Comments

```cpp
// ✅ GOOD: Explains WHY (not obvious)
// Using spatial hash instead of quadtree for cache locality in tight loop
class SpatialHash { };

// ❌ BAD: Obvious from code
// Increments i
i++;

// ✅ GOOD: Documents algorithm choice
// A* pathfinding with Manhattan heuristic for 2D tilemaps
// (Euclidean is overkill here, Manhattan is 10% faster)
std::vector<vec2> FindPath(vec2 start, vec2 goal);
```

---

## QUALITY GATES

### Pre-Commit Checks (Run Locally)

```bash
# ✅ MUST PASS before commit
✓ No compile errors
✓ No compile warnings (treat as errors: -Werror)
✓ No formatting violations (clang-format)
✓ All unit tests pass
✓ Static analysis (clang-tidy) clean
```

### Pre-PR Checks (GitHub Actions)

```bash
✓ Full build (Debug + Release)
✓ Unit test coverage > 80%
✓ No regressions on previous phases
✓ Doxygen builds without warnings
✓ Code style check
```

### Code Review Checklist

**Architecture:**
- [ ] Follows ECS principles (no God objects)
- [ ] Components are data-only
- [ ] Systems are cohesive
- [ ] No circular dependencies
- [ ] Clear ownership model

**Code Quality:**
- [ ] Naming follows conventions
- [ ] Functions < 50 lines
- [ ] No duplicate code (DRY)
- [ ] Error handling present
- [ ] Const correctness
- [ ] Memory safety (smart pointers)

**Testing:**
- [ ] Unit tests added
- [ ] > 80% coverage on new code
- [ ] Round-trip tests if JSON modified
- [ ] Integration tests if multi-system change

**Documentation:**
- [ ] Public API has Doxygen comments
- [ ] Tricky code explained
- [ ] Architecture decisions logged
- [ ] README updated if needed

---

## CODE REVIEW CHECKLIST

### Template for PR Description

```markdown
## What This PR Does
- [Brief description]

## Architecture Decisions
- [Design choices made]

## Testing
- [ ] Unit tests added
- [ ] Coverage: X%
- [ ] Manual testing done

## Checklist
- [ ] Follows C++14 standards
- [ ] No compiler warnings
- [ ] Doxygen comments added
- [ ] Code review checklist passed
```

---

## PERFORMANCE BENCHMARKS

### Frame Budget (60 FPS target)

| System | Budget | Current | Status |
|--------|--------|---------|--------|
| **Physics** | 5 ms | 2-3 ms | ✅ OK |
| **Rendering** | 8 ms | 3-4 ms | ✅ OK |
| **Scripts** | 3 ms | 1-2 ms | ✅ OK |
| **Total** | 16 ms | 6-9 ms | ✅ OK |

### Memory Targets

| Resource | Budget | Current | Status |
|----------|--------|---------|--------|
| **Entity Pool** | 100 MB | 50 MB | ✅ OK |
| **Component Data** | 50 MB | 20 MB | ✅ OK |
| **Textures** | 200 MB | 80 MB | ✅ OK |
| **Total Runtime** | 400 MB | 250 MB | ✅ OK |

### Profiling Checklist

Before release, verify:
- [ ] No memory leaks (valgrind/AddressSanitizer)
- [ ] No hot spots (flame graph)
- [ ] Cache misses acceptable (perf profiler)
- [ ] 60 FPS sustained on target hardware

---

## QUICK REFERENCE

### What Gets Rejected

❌ Compile errors/warnings  
❌ Missing unit tests  
❌ Coverage < 60%  
❌ No Doxygen comments  
❌ Violates naming conventions  
❌ > 100 line functions  
❌ Global mutable state  
❌ Exception handling  
❌ Raw `new/delete` (no smart pointers)  

### What Gets Approved

✅ Clean, readable code  
✅ > 80% test coverage  
✅ Doxygen documented  
✅ Passes all gates  
✅ Follows patterns  
✅ Performance acceptable  
✅ Memory safe  
✅ Error handling present  

---

**Last Updated:** 2026-03-19  
**Next Review:** Phase 25  
**Enforced By:** COPILOT_CODING_AGENT + Code Review

