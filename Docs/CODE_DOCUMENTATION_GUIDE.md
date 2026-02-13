# Code Documentation Guide

## Doxygen Comment Syntax

### Class Documentation

```cpp
/**
 * @brief Component storing entity health data
 * 
 * This component tracks an entity's current and maximum health values.
 * It is used by the CombatSystem and HealthSystem for damage calculation.
 * 
 * @see CombatSystem
 * @see HealthSystem
 */
struct Health_data {
    /**
     * @brief Current health points
     * 
     * Must be between 0 and maxHealth. When reaching 0,
     * the entity is considered dead.
     */
    float currentHealth = 100.0f;
    
    /**
     * @brief Maximum health capacity
     */
    float maxHealth = 100.0f;
    
    /**
     * @brief Check if entity is alive
     * @return true if currentHealth > 0
     */
    bool IsAlive() const { return currentHealth > 0.0f; }
};
```

### Function Documentation

```cpp
/**
 * @brief Create entity from prefab blueprint
 * 
 * Instantiates a new entity by loading and applying all components
 * defined in the prefab blueprint file.
 * 
 * @param prefabName Name of the prefab (without .json extension)
 * @return EntityID of created entity, or INVALID_ENTITY_ID on failure
 * 
 * @throws std::runtime_error if prefab file is not found
 * 
 * @example
 * ```cpp
 * EntityID enemy = PrefabFactory::Get().CreateEntityFromPrefabName("Enemy_Guard");
 * ```
 * 
 * @see PrefabBlueprint
 * @see InstantiateComponent
 */
EntityID CreateEntityFromPrefabName(const std::string& prefabName);
```

### File Documentation

```cpp
/**
 * @file ECS_Components.h
 * @brief Core ECS component definitions
 * 
 * This file contains all component data structures used in the
 * Entity Component System architecture.
 * 
 * @author Nicolas Chereau
 * @date 2025
 */
```

## Best Practices

1. **Always document public APIs**
2. Use `@brief` for every class/struct/function
3. Document all parameters with `@param`
4. Document return values with `@return`
5. Add `@example` for complex functions
6. Use `@warning` for gotchas
7. Cross-reference with `@see`

## Checking Your Documentation

Run Doxygen locally:
```bash
doxygen Doxyfile
# Open website/static/api/html/index.html in browser
```
