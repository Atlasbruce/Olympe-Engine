# EventManager Template Registration

## Overview

The `EventManager` now supports registering **any class** with an `OnEvent(const Message&)` method, not just classes that inherit from `Object`.

## Technical Details

### SFINAE Implementation

The template uses `std::enable_if` with SFINAE (Substitution Failure Is Not An Error) to avoid conflicts:

```cpp
template<typename T>
typename std::enable_if<!std::is_base_of<Object, T>::value, void>::type
Register(T* owner, EventType type)
```

**How it works:**
- If `T` inherits from `Object` → uses existing `Register(Object* obj, EventType type)` method
- If `T` does NOT inherit from `Object` → uses the template method
- Compile-time verification via `static_assert` ensures `T` has an `OnEvent` method

### Advantages

✅ **Backward Compatible**: All existing `Object`-based classes continue to work  
✅ **Type Safe**: Compile-time checking with `static_assert`  
✅ **Flexible**: Any class can register for events  
✅ **No Refactoring**: Existing code doesn't need changes  

## Usage Examples

### Example 1: Custom Listener (Non-Object Class)

```cpp
#include "system/EventManager.h"

class MyCustomListener
{
public:
    void OnEvent(const Message& msg) 
    {
        if (msg.msg_type == EventType::Olympe_EventType_Custom)
        {
            // Handle custom event
            std::cout << "Custom event received!" << std::endl;
        }
    }
};

// Usage
MyCustomListener listener;
EventManager::Get().Register(&listener, EventType::Olympe_EventType_Custom);

// Don't forget to unregister when done
EventManager::Get().Unregister(&listener, EventType::Olympe_EventType_Custom);
```

### Example 2: Object-based Listener (Still Works)

```cpp
#include "system/EventManager.h"
#include "Object.h"

class MyObjectListener : public Object
{
public:
    void OnEvent(const Message& msg) override
    {
        // Handle event
    }
};

// Usage - same as before
MyObjectListener listener;
EventManager::Get().Register(&listener, EventType::Olympe_EventType_Custom);
```

### Example 3: Game-Specific Listener

```cpp
// Example: Audio system that doesn't need to inherit from Object
class AudioSystem
{
private:
    bool m_muted = false;
    
public:
    void OnEvent(const Message& msg)
    {
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Audio_Mute:
                m_muted = true;
                break;
            case EventType::Olympe_EventType_Audio_Unmute:
                m_muted = false;
                break;
            default:
                break;
        }
    }
};

// Register for multiple event types
AudioSystem audioSystem;
EventManager::Get().Register(&audioSystem, EventType::Olympe_EventType_Audio_Mute);
EventManager::Get().Register(&audioSystem, EventType::Olympe_EventType_Audio_Unmute);
```

## Compilation Error Handling

### Missing OnEvent Method

If you try to register a class without an `OnEvent` method, you'll get a clear compile-time error:

```cpp
class InvalidListener
{
    // No OnEvent method
};

InvalidListener listener;
EventManager::Get().Register(&listener, EventType::Olympe_EventType_Custom);
// ❌ Compilation error: "Type T must have a member function OnEvent(const Message&)"
```

### Wrong OnEvent Signature

If your `OnEvent` method has the wrong signature, the compiler will catch it when instantiating the lambda:

```cpp
class WrongSignature
{
public:
    void OnEvent(int value) { /* wrong signature */ }
};

WrongSignature listener;
EventManager::Get().Register(&listener, EventType::Olympe_EventType_Custom);
// ❌ Compilation error: lambda cannot call OnEvent with Message parameter
```

**Note**: The `static_assert` checks for the existence of an `OnEvent` member function. While it doesn't validate the exact signature `OnEvent(const Message&)`, any signature mismatch will be caught at compile-time when the lambda attempts to call `owner->OnEvent(msg)`.

## Best Practices

1. **Always unregister listeners** when they are destroyed to avoid dangling pointers:
   ```cpp
   EventManager::Get().UnregisterAll(&listener);
   ```

2. **Use RAII pattern** for automatic cleanup:
   ```cpp
   class ScopedListener
   {
   public:
       ScopedListener(EventType type) : m_type(type)
       {
           EventManager::Get().Register(this, m_type);
       }
       
       ~ScopedListener()
       {
           EventManager::Get().Unregister(this, m_type);
       }
       
       void OnEvent(const Message& msg) { /* ... */ }
       
   private:
       EventType m_type;
   };
   ```

3. **Prefer composition over inheritance** when you don't need the full Object interface:
   ```cpp
   // Good: Lightweight listener
   class UIEventHandler
   {
   public:
       void OnEvent(const Message& msg) { /* ... */ }
   };
   
   // Overkill if you only need event handling
   class UIEventHandler : public Object { /* ... */ };
   ```

## Implementation Notes

### Files Modified

- `Source/system/EventManager.h`:
  - Added `#include <type_traits>` (line 11)
  - Added template `Register` method (lines 118-131)

### C++ Standard

The implementation uses C++11 and C++14 features:
- C++11: `std::enable_if`, `std::is_base_of`, `std::is_member_function_pointer`, `static_assert`
- C++14: Generic lambdas (lambda with auto parameters not used here, but available)

The code is compatible with C++14 standard as used by the Olympe Engine project.

### Thread Safety

The template method uses the same thread-safe registration mechanism as the Object* overload:
- Protected by `m_listenersMutex`
- Safe to call from multiple threads

## Migration Guide

No migration needed! This is a pure addition:

- Existing code continues to work unchanged
- New code can use the template for non-Object classes
- No breaking changes

## See Also

- `Source/system/EventManager.h` - Full implementation
- `Source/system/message.h` - Message structure definition
- `Source/Object.h` - Object base class
