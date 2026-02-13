# Component Auto-Registration System

## Overview
The Olympe Engine uses a static dummy variable pattern for automatic component registration in the `PrefabFactory`. This eliminates manual switch-case maintenance and makes adding new components trivial.

## Adding a New Component

### Step 1: Define the component struct
```cpp
// In ECS_Components.h or ECS_Components_AI.h

struct MyNewComponent_data
{
    float value = 0.0f;
    std::string name;
    
    MyNewComponent_data() = default;
};
```

### Step 2: Add auto-registration macro immediately after
```cpp
AUTO_REGISTER_COMPONENT(MyNewComponent_data);
```

**That's it!** The component is now:
- ✅ Registered at program startup (before `main()`)
- ✅ Available in all prefab JSON files
- ✅ Instantiated automatically by `PrefabFactory`

## Verification

At program startup, you'll see:
```
[ComponentRegistry] Registered: MyNewComponent_data
```

To list all registered components:
```cpp
auto components = PrefabFactory::Get().GetRegisteredComponents();
for (const auto& name : components) {
    std::cout << "  - " << name << "\n";
}
```

## How It Works

The `AUTO_REGISTER_COMPONENT()` macro creates a static dummy variable:

```cpp
static ComponentRegistrar_MyNewComponent_data g_registrar_MyNewComponent_data;
```

Its constructor executes **before `main()`**, registering the component in the factory.

## Best Practices

1. **Always place `AUTO_REGISTER_COMPONENT()` immediately after struct definition**
   - Makes it visually obvious
   - Impossible to forget

2. **Keep the pattern consistent**
   ```cpp
   struct ComponentA_data { /* ... */ };
   AUTO_REGISTER_COMPONENT(ComponentA_data);
   
   struct ComponentB_data { /* ... */ };
   AUTO_REGISTER_COMPONENT(ComponentB_data);
   ```

3. **Verify registration at startup**
   - Check console logs for `[ComponentRegistry] Registered: ...`
   - If a component is missing, you forgot the macro!

## Specialized Parameter Handling

For components needing custom parameter parsing (e.g., `BehaviorTreeRuntime_data`), keep the specialized `InstantiateXYZ()` function but let auto-registration handle creation:

```cpp
// Auto-registration creates the component
AUTO_REGISTER_COMPONENT(BehaviorTreeRuntime_data);

// Specialized function applies complex parameters
bool PrefabFactory::InstantiateBehaviorTreeRuntime(EntityID entity, const ComponentDefinition& def)
{
    // Component already created by auto-registration
    BehaviorTreeRuntime_data& runtime = World::Get().GetComponent<BehaviorTreeRuntime_data>(entity);
    
    // Apply specialized parameters
    const auto* treePathParam = def.GetParameter("treePath");
    if (treePathParam)
    {
        std::string treePath = treePathParam->AsString();
        runtime.treeAssetId = BehaviorTreeManager::Get().GetTreeIdFromPath(treePath);
    }
    
    return true;
}
```

## Troubleshooting

### "Unknown component type 'XYZ_data'"
→ You forgot `AUTO_REGISTER_COMPONENT(XYZ_data)` after the struct definition

### Component not in list at startup
→ Check that `#include "ComponentRegistry.h"` is at the top of the file
→ Verify the macro is placed **after** the struct definition, not inside

### Linker errors
→ Make sure `ComponentRegistry.h` is included in the translation unit
→ Verify `PrefabFactory.cpp` implements `RegisterComponentFactory_Internal()`

## Benefits

✅ **1 line per component** (macro after struct)  
✅ **No central list** to maintain  
✅ **Impossible to forget** (visual pattern)  
✅ **Auto-discovery** at startup  
✅ **Backward compatible** (existing code unchanged)  
✅ **Standard C++** (works on all compilers)  
✅ **Scalable** (works with 10 or 100+ components)  

## Migration Notes

Existing specialized `InstantiateXYZ()` functions are preserved for:
- Complex parameter parsing (e.g., BehaviorTreeRuntime)
- Backward compatibility
- Gradual migration

New components can simply use `AUTO_REGISTER_COMPONENT()` without needing specialized functions.
