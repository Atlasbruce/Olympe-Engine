/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

ComponentRegistry: Auto-registration system for ECS components

Purpose:
- Automatic registration of ECS components in PrefabFactory
- Eliminates manual switch-case maintenance
- Pattern: Place AUTO_REGISTER_COMPONENT() immediately after each struct definition

Usage:
    struct MyComponent_data {
        float value = 0.0f;
    };
    AUTO_REGISTER_COMPONENT(MyComponent_data);

Implementation Notes:
- Uses static initialization (before main()) for auto-registration
- Safe because: no cross-component dependencies, singleton pattern, simple registration
- Keep component constructors simple (default initialization only)
- Complex initialization should be done in specialized InstantiateXYZ() functions
*/

#pragma once
#include "World.h"
#include "ComponentDefinition.h"
#include <functional>
#include <iostream>

// Forward declaration
class PrefabFactory;

//==============================================================================
// GENERIC COMPONENT INSTANTIATION
//==============================================================================

/// Generic template function for instantiating any ECS component
/// This handles the common case: create component if missing, get reference
/// Specialized behavior can still use custom InstantiateXYZ() functions
/// 
/// @return true if component was created successfully, false otherwise
///         Note: Currently always returns true for generic components.
///         Custom factory functions can return false to indicate failure.
template<typename T>
bool InstantiateComponentGeneric(EntityID entity, const ComponentDefinition& def)
{
    // Create component if it doesn't exist
    if (!World::Get().HasComponent<T>(entity))
    {
        World::Get().AddComponent<T>(entity);
    }
    
    // Get component reference
    T& component = World::Get().GetComponent<T>(entity);
    
    // Note: Parameter application is handled by specialized functions
    // or can be extended here with reflection/traits
    
    return true;
}

//==============================================================================
// AUTO-REGISTRATION MACRO
//==============================================================================

/// Macro for auto-registering a component at program startup
/// Place this immediately after the struct definition
/// 
/// Example:
///     struct Position_data { Vector position; };
///     AUTO_REGISTER_COMPONENT(Position_data);
///
/// This creates a static dummy variable whose constructor executes before main()
#define AUTO_REGISTER_COMPONENT(ComponentType) \
    namespace { \
        struct ComponentRegistrar_##ComponentType { \
            ComponentRegistrar_##ComponentType(); \
        }; \
        static ComponentRegistrar_##ComponentType g_registrar_##ComponentType; \
    } \
    inline ComponentRegistrar_##ComponentType::ComponentRegistrar_##ComponentType() { \
        extern void RegisterComponentFactory_Internal(const char*, std::function<bool(EntityID, const ComponentDefinition&)>); \
        RegisterComponentFactory_Internal( \
            #ComponentType, \
            [](EntityID entity, const ComponentDefinition& def) { \
                return InstantiateComponentGeneric<ComponentType>(entity, def); \
            } \
        ); \
    }
