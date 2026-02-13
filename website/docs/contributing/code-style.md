---
id: code-style
title: "Code Style Guide"
sidebar_label: "Code Style"
---

# Code Style Guide

This document outlines the coding standards and best practices for contributing to Olympe Engine. Following these guidelines ensures consistency and maintainability across the codebase.

## C++ Standard

Olympe Engine uses **C++14** as its standard:

```cmake
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

- Use C++14 features where appropriate
- Avoid C++17/20 features for compatibility
- Prefer standard library over custom implementations when available

## Naming Conventions

### Classes and Structs

- **PascalCase** for class and struct names
- Descriptive, full words (no abbreviations unless widely known)

```cpp
class GameEngine { };
class InputsManager { };
struct Position_data { };
struct AIBlackboard_data { };
```

### Component Data Structures

- Suffix component structures with `_data`
- Use PascalCase for the base name

```cpp
struct Identity_data { };
struct Position_data { };
struct Velocity_data { };
struct VisualSprite_data { };
```

### Systems

- Suffix system classes with `System`
- Use PascalCase

```cpp
class InputSystem : public ECS_System { };
class AISystem : public ECS_System { };
class RenderingSystem : public ECS_System { };
```

### Functions and Methods

- **camelCase** for functions and methods
- Use descriptive verb-noun combinations

```cpp
void Initialize();
void Process();
EntityID CreateEntity();
void AddComponent<T>(EntityID entity, T component);
SDL_Texture* LoadTexture(const std::string& path);
```

- **Public API**: Clear, self-documenting names
- **Private helpers**: Prefix with lowercase or use similar conventions

### Member Variables

- **camelCase** for member variables
- Prefix with `m_` for private members (optional but recommended)
- No prefix for public members

```cpp
class GameEngine {
public:
    float deltaTime;
    SDL_Renderer* renderer;
    
private:
    std::vector<EntityID> m_entities;
    bool m_isRunning;
};
```

- **Pointer members**: Prefix with `ptr_` for clarity (used in some legacy code)

```cpp
InputsManager* ptr_inputsmanager = nullptr;
VideoGame* ptr_videogame = nullptr;
```

### Constants and Enums

- **UPPER_SNAKE_CASE** for global constants
- **PascalCase** for enum class names
- **PascalCase** for enum values

```cpp
const int MAX_COMPONENTS = 64;
const float DEFAULT_SPEED = 100.0f;

enum class EntityType : int {
    None = 0,
    Player,
    NPC,
    Enemy,
    Item
};

enum class RenderLayer : int {
    Background_Far = -2,
    Background_Near = -1,
    Ground = 0,
    Characters = 2
};
```

### Local Variables

- **camelCase** for local variables
- Short, descriptive names

```cpp
EntityID entity = CreateEntity();
Position_data* position = GetComponent<Position_data>(entity);
float deltaTime = GameEngine::fDt;
```

### Namespaces

- **PascalCase** for namespace names
- Nested namespaces for organization

```cpp
namespace Olympe {
    namespace Tiled {
        struct TiledMap { };
    }
    
    namespace Editor {
        class BlueprintEditor { };
    }
}
```

## Code Formatting

### Indentation

- **Tabs** (not spaces) for indentation
- Tab width: 4 spaces equivalent
- Consistent indentation throughout file

```cpp
void Process() {
	for (EntityID entity : m_entities) {
		Position_data* pos = GetComponent<Position_data>(entity);
		if (pos) {
			pos->position.x += 10.0f;
		}
	}
}
```

### Braces

- **Opening brace on same line** for functions, classes, and control structures
- Closing brace on separate line

```cpp
class GameEngine {
public:
    void Process() {
        if (isRunning) {
            // Code here
        }
    }
};
```

### Spacing

- Space after control keywords: `if (`, `for (`, `while (`
- No space between function name and parentheses: `Function()`
- Space around binary operators: `a + b`, `x = 5`
- No space before semicolons

```cpp
// Good
if (condition) {
    result = a + b * c;
}

// Bad
if(condition){
    result=a+b*c ;
}
```

### Line Length

- Aim for **80-120 characters** per line
- Break long lines logically

```cpp
// Good
EntityID player = PrefabFactory::CreateEntityFromBlueprint(
    "Blueprints/EntityPrefab/player.json"
);

// Acceptable for short statements
EntityID player = PrefabFactory::CreateEntityFromBlueprint("Blueprints/EntityPrefab/player.json");
```

## Header Files

### Include Guards

- Use `#pragma once` for header guards (simpler and widely supported)

```cpp
#pragma once

class GameEngine {
    // ...
};
```

### Include Order

1. Corresponding header (for .cpp files)
2. Engine headers
3. Third-party library headers
4. Standard library headers

```cpp
// GameEngine.cpp
#include "GameEngine.h"

#include "InputsManager.h"
#include "World.h"
#include "VideoGame.h"

#include <SDL3/SDL.h>
#include "third_party/nlohmann/json.hpp"

#include <vector>
#include <string>
#include <memory>
```

### Forward Declarations

- Use forward declarations to minimize dependencies
- Only include headers when necessary

```cpp
// Header file
#pragma once

// Forward declarations
class InputsManager;
class VideoGame;
class DataManager;

class GameEngine {
    InputsManager* ptr_inputsmanager;
    VideoGame* ptr_videogame;
    DataManager* ptr_datamanager;
};
```

## Comments and Documentation

### File Headers

- Include copyright, author, and purpose at the top of each file

```cpp
/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Purpose: Core game engine class that orchestrates subsystems
and manages the main game loop.
*/

#pragma once
```

### Doxygen Comments

- Use Doxygen-style comments for public APIs
- Include `@brief`, `@param`, `@return` tags

```cpp
/**
 * @brief Creates a new entity in the world
 * @return EntityID of the newly created entity
 */
EntityID CreateEntity();

/**
 * @brief Adds a component to an entity
 * @tparam T Component type
 * @param entity The entity to add the component to
 * @param component The component data to add
 */
template<typename T>
void AddComponent(EntityID entity, T component);
```

### Inline Comments

- Use `//` for single-line comments
- Comment complex logic, not obvious code
- Explain "why", not "what"

```cpp
// Good - explains reasoning
// Use delta time for frame-rate independent movement
pos->position.x += velocity.x * GameEngine::fDt;

// Bad - states the obvious
// Add velocity to position
pos->position.x += velocity.x;
```

### Section Comments

- Use comment blocks to separate major sections

```cpp
// ========================================================================
// Entity Management
// ========================================================================

EntityID CreateEntity() { /* ... */ }
void DestroyEntity(EntityID entity) { /* ... */ }

// ========================================================================
// Component Management
// ========================================================================

template<typename T>
void AddComponent(EntityID entity, T component) { /* ... */ }
```

## Best Practices

### Modern C++ Features

#### Use `auto` Judiciously

```cpp
// Good - type is obvious from right side
auto entity = World::Get().CreateEntity();
auto* texture = DataManager::Get().LoadTexture("sprite.png");

// Bad - type unclear
auto x = CalculateComplexValue();
```

#### Range-Based For Loops

```cpp
// Prefer range-based loops
for (EntityID entity : m_entities) {
    Process(entity);
}

// Over traditional loops
for (size_t i = 0; i < m_entities.size(); ++i) {
    Process(m_entities[i]);
}
```

#### Prefer `nullptr` Over `NULL`

```cpp
// Good
SDL_Texture* texture = nullptr;

// Bad
SDL_Texture* texture = NULL;
```

#### Use `enum class` Over `enum`

```cpp
// Good - type-safe, scoped
enum class EntityType : int {
    Player,
    Enemy,
    NPC
};

// Bad - global scope pollution
enum EntityType {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ENEMY
};
```

### Memory Management

#### Smart Pointers

- Use `std::unique_ptr` for single ownership
- Use `std::shared_ptr` sparingly (only when truly needed)
- Avoid raw `new`/`delete` when possible

```cpp
// Good
std::unique_ptr<BlueprintEditor> editor = std::make_unique<BlueprintEditor>();

// Acceptable for non-owning pointers
Position_data* pos = GetComponent<Position_data>(entity);
```

#### RAII (Resource Acquisition Is Initialization)

- Acquire resources in constructors
- Release resources in destructors
- Let destructors handle cleanup

```cpp
class ResourceManager {
public:
    ResourceManager(const std::string& path) {
        m_data = LoadData(path);
    }
    
    ~ResourceManager() {
        FreeData(m_data);
    }
    
private:
    void* m_data;
};
```

### Error Handling

- Use return values or exceptions for error handling
- Log errors using `SYSTEM_LOG`
- Check pointer validity before dereferencing

```cpp
Position_data* pos = GetComponent<Position_data>(entity);
if (pos) {
    pos->position.x += 10.0f;
} else {
    SYSTEM_LOG << "Error: Entity " << entity << " has no Position component\n";
}
```

### Const Correctness

- Use `const` for parameters that shouldn't be modified
- Mark methods `const` if they don't modify state

```cpp
class Vector {
public:
    float Length() const;  // Doesn't modify vector
    void Normalize();      // Modifies vector
};

void Process(const Vector& position) {  // Won't modify position
    float length = position.Length();
}
```

### Singletons

- Use the standard singleton pattern with static instance
- Provide both `GetInstance()` and `Get()` accessors

```cpp
class GameEngine {
public:
    static GameEngine& GetInstance() {
        static GameEngine instance;
        return instance;
    }
    
    static GameEngine& Get() { return GetInstance(); }
    
private:
    GameEngine() = default;
    ~GameEngine() = default;
    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
};
```

## File Organization

### Source Structure

```
Source/
├── Core/                   # Core engine classes
│   ├── GameEngine.h/cpp
│   ├── World.h/cpp
│   └── VideoGame.h/cpp
├── ECS_*.h/cpp            # ECS architecture files
├── AI/                    # AI and behavior trees
├── Rendering/             # Rendering systems
├── system/                # Low-level utilities
├── third_party/           # External libraries
└── BlueprintEditor/       # Editor code
```

### Header/Implementation Split

- Declare classes in `.h` files
- Implement methods in `.cpp` files
- Template implementations stay in headers

```cpp
// GameEngine.h
#pragma once

class GameEngine {
public:
    void Process();
private:
    float m_deltaTime;
};

// GameEngine.cpp
#include "GameEngine.h"

void GameEngine::Process() {
    // Implementation
}
```

## Testing and Validation

### Build Before Committing

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Code Reviews

- Submit pull requests for all changes
- Address reviewer feedback promptly
- Keep PRs focused and reasonably sized

### Documentation

- Update documentation when changing public APIs
- Add examples for new features
- Document breaking changes in PR descriptions

## Tools and Linters

### Recommended Tools

- **Visual Studio 2019+** or **VS Code** with C++ extensions
- **CMake 3.14+** for build configuration
- **Doxygen** for API documentation generation

### Code Analysis

- Enable compiler warnings (`/W4` on MSVC, `-Wall` on GCC/Clang)
- Fix warnings before committing
- Use static analysis tools when available

## Common Patterns in Olympe Engine

### Entity Creation

```cpp
EntityID CreatePlayer() {
    EntityID player = World::Get().CreateEntity();
    
    Identity_data identity;
    identity.name = "Player";
    identity.entityType = EntityType::Player;
    World::Get().AddComponent<Identity_data>(player, identity);
    
    Position_data position;
    position.position = Vector(0.0f, 0.0f, 0.0f);
    World::Get().AddComponent<Position_data>(player, position);
    
    return player;
}
```

### System Implementation

```cpp
class MySystem : public ECS_System {
public:
    MySystem() {
        requiredSignature.set(World::GetComponentTypeID<Position_data>());
        requiredSignature.set(World::GetComponentTypeID<Velocity_data>());
    }
    
    virtual void Process() override {
        for (EntityID entity : m_entities) {
            auto* pos = World::Get().GetComponent<Position_data>(entity);
            auto* vel = World::Get().GetComponent<Velocity_data>(entity);
            
            pos->position = pos->position + vel->velocity * GameEngine::fDt;
        }
    }
};
```

### Singleton Access

```cpp
// In implementation files
GameEngine& engine = GameEngine::Get();
World& world = World::Get();
InputsManager& input = InputsManager::Get();
```

## Questions?

If you're unsure about style decisions:

1. Check existing code for similar patterns
2. Prioritize readability and maintainability
3. When in doubt, ask in pull request comments
4. Follow the principle of least surprise

## Summary

- **C++14 standard**
- **PascalCase** for classes, structs, enums
- **camelCase** for functions, variables
- **Tabs** for indentation
- **`#pragma once`** for header guards
- **Const correctness**
- **Smart pointers** over raw pointers
- **Document public APIs** with Doxygen
- **Comment complex logic**, not obvious code

Thank you for contributing to Olympe Engine!
