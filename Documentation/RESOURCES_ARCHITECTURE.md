# Resources Architecture

## Overview

The **Resources** architecture provides a centralized registry of global resources for ECS systems, eliminating implicit singleton access and making dependencies explicit.

## Motivation

Previously, ECS systems accessed global resources through singleton patterns:
- `GameEngine::renderer` - SDL renderer
- `GameEngine::fDt` - Delta time
- `World::Get()` - Component access
- `CameraManager::Get()` - Camera operations
- `InputsManager::Get()` - Input state
- etc.

This approach had several issues:
- **Hidden dependencies**: Systems had implicit global dependencies
- **Testing difficulty**: Hard to test systems in isolation
- **Thread safety**: Global state makes concurrent execution problematic
- **Multi-instance**: Impossible to run multiple engine instances

## Solution: Explicit Resources Registry

The `Resources` struct centralizes all global resources and is passed explicitly to each system:

```cpp
struct Resources
{
    // SDL Rendering Context
    SDL_Renderer* renderer = nullptr;
    
    // Timing
    float deltaTime = 0.0f;
    
    // Display Configuration
    int screenWidth = 640;
    int screenHeight = 320;
    
    // System Managers
    EventManager* eventManager = nullptr;
    InputsManager* inputsManager = nullptr;
    CameraManager* cameraManager = nullptr;
    KeyboardManager* keyboardManager = nullptr;
    JoystickManager* joystickManager = nullptr;
    MouseManager* mouseManager = nullptr;
    DataManager* dataManager = nullptr;
    
    // ECS World
    World* world = nullptr;
};
```

## Architecture

### ECS_System Base Class

All systems inherit from `ECS_System` which now includes:

```cpp
class ECS_System
{
protected:
    Resources* m_resources;  // Non-owning pointer to resources
    
public:
    void SetResources(Resources* resources);
    Resources* GetResources() const;
};
```

### World Class

The `World` class owns and manages the `Resources` instance:

```cpp
class World
{
private:
    Resources m_resources;  // Owned by World
    
public:
    Resources& GetResources();
    void Add_ECS_System(std::unique_ptr<ECS_System> system);
};
```

### System Initialization Flow

1. **World Constructor**: Initializes `Resources` with singleton references
   ```cpp
   World::World()
   {
       m_resources.renderer = GameEngine::renderer;
       m_resources.deltaTime = GameEngine::fDt;
       m_resources.eventManager = &EventManager::Get();
       // ... initialize all resources
   }
   ```

2. **System Registration**: Resources passed to each system
   ```cpp
   void World::Add_ECS_System(std::unique_ptr<ECS_System> system)
   {
       system->SetResources(&m_resources);
       m_systems.push_back(std::move(system));
   }
   ```

3. **Per-Frame Update**: Resources refreshed with current values
   ```cpp
   void World::Process_ECS_Systems()
   {
       m_resources.deltaTime = GameEngine::fDt;
       m_resources.renderer = GameEngine::renderer;
       
       for (const auto& system : m_systems)
           system->Process();
   }
   ```

## System Implementation

### Before (Implicit Dependencies)

```cpp
void MovementSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        Movement_data& move = World::Get().GetComponent<Movement_data>(entity);
        pos.position += move.direction * GameEngine::fDt;
    }
}
```

### After (Explicit Dependencies)

```cpp
void MovementSystem::Process()
{
    if (!m_resources || !m_resources->world)
        return;
    
    for (EntityID entity : m_entities)
    {
        Position_data& pos = m_resources->world->GetComponent<Position_data>(entity);
        Movement_data& move = m_resources->world->GetComponent<Movement_data>(entity);
        pos.position += move.direction * m_resources->deltaTime;
    }
}
```

## Benefits

### 1. Explicit Dependencies
- Clear what resources each system needs
- Easy to see and modify dependencies
- Better code documentation

### 2. Testing
- Systems can be tested with mock resources
- No need for global state setup
- Isolated unit tests possible

### 3. Debugging
- Resource access is traceable
- Null checks prevent crashes
- Clear error messages

### 4. Future-Proofing
- Network/multiplayer support easier
- Multi-instance engines possible
- Thread-safe execution feasible

## Updated Systems

The following systems have been updated to use Resources:

- ✅ **MovementSystem** - Uses `deltaTime` and `world`
- ✅ **RenderingSystem** - Uses `renderer`, `cameraManager`, and `world`
- ✅ **InputMappingSystem** - Uses `inputsManager`, `keyboardManager`, `joystickManager`, and `world`
- ✅ **PlayerControlSystem** - Uses `deltaTime` and `world`
- ✅ **OlympeEffectSystem** - Uses `renderer`, `deltaTime`, `cameraManager`, and `dataManager`

## Backward Compatibility

**Singletons still exist** for backward compatibility with non-ECS code. The key change is:
- Singletons are **only accessed by World** to populate Resources
- Systems **only access Resources**, not singletons directly
- This allows gradual migration of remaining code

## Future Work

1. **Expand to non-ECS systems**: Apply pattern to other engine subsystems
2. **Remove remaining singletons**: Once all code uses Resources, remove GetInstance()
3. **Thread-safe Resources**: Add synchronization for concurrent access
4. **Resource validation**: Add runtime checks for resource availability
5. **Resource scoping**: Allow different resource sets for different contexts

## Examples

### Creating a New System

```cpp
class MySystem : public ECS_System
{
public:
    MySystem()
    {
        requiredSignature.set(GetComponentTypeID_Static<MyComponent>(), true);
    }
    
    virtual void Process() override
    {
        // Always check resources first
        if (!m_resources || !m_resources->world)
            return;
        
        // Use resources explicitly
        for (EntityID entity : m_entities)
        {
            MyComponent& comp = m_resources->world->GetComponent<MyComponent>(entity);
            comp.Update(m_resources->deltaTime);
        }
    }
};
```

### Testing a System

```cpp
void TestMySystem()
{
    // Setup mock resources
    Resources testResources;
    World testWorld;
    testResources.world = &testWorld;
    testResources.deltaTime = 0.016f; // 60 FPS
    
    // Create and test system
    auto system = std::make_unique<MySystem>();
    system->SetResources(&testResources);
    system->Process();
    
    // Verify results
    // ...
}
```

## Migration Guide

To convert an existing system to use Resources:

1. **Identify dependencies**: List all singletons/globals used
2. **Add null checks**: Check `m_resources` at function start
3. **Replace singleton calls**: Use `m_resources->X` instead of `X::Get()`
4. **Update GameEngine statics**: Use `m_resources->deltaTime` instead of `GameEngine::fDt`
5. **Test thoroughly**: Ensure system still works correctly

## Conclusion

The Resources architecture provides a clean, testable, and maintainable approach to resource management in the ECS. By making dependencies explicit, we improve code quality and enable future enhancements like multi-instance engines and thread-safe execution.
