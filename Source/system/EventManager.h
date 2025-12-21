#pragma once

// DEPRECATED: EventManager has been replaced by EventQueue with ECS consumer systems
// This stub exists only for legacy GameObject-based code compatibility
// DO NOT USE in new code - use EventQueue and consumer systems instead

#include "message.h"
#include <functional>

// Legacy EventManager stub for backwards compatibility with non-critical legacy code
// (AI_Player, ObjectFactory, PanelManager, etc.)
// These systems are being phased out in favor of ECS architecture
class EventManager 
{
public:
    using Message = ::Message;
    using Listener = std::function<void(const Message&)>;

    EventManager() {}
    ~EventManager() {}

    // Singleton accessor
    static EventManager& GetInstance()
    {
        static EventManager instance;
        return instance;
    }
    static EventManager& Get() { return GetInstance(); }

    // Stub methods - do nothing (legacy code only)
    void AddMessage(const Message& msg) { /* deprecated - no-op */ }
    void DispatchImmediate(const Message& msg) { /* deprecated - no-op */ }
    void Process() { /* deprecated - no-op */ }
    
    void Register(void* owner, EventType type, Listener callback) { /* deprecated - no-op */ }
    
    template<typename T>
    void Register(T* owner, EventType type) { /* deprecated - no-op */ }
    
    void Unregister(void* owner, EventType type) { /* deprecated - no-op */ }
    void UnregisterAll(void* owner) { /* deprecated - no-op */ }
};
