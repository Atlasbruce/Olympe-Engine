#pragma once

//#include "../object.h"
#include "message.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include <algorithm>
#include <type_traits>
#include "system_utils.h"

// Forward declaration of helper implemented in EventManager_ECS.cpp
void UpdateECSInputFromMessage(const Message& msg);

class EventManager 
{
public:
    using Message = ::Message;
    using Listener = std::function<void(const Message&)>;
    using ListenerEntry = std::pair<void*, Listener>; // owner pointer + callback

public:
    EventManager()
    {
		name = "EventManager";
        SYSTEM_LOG << "EventManager created and Initialized\n";
    }

    ~EventManager()
    {
        SYSTEM_LOG << "EventManager destroyed\n";
	}

    // Per-class singleton accessors
    static EventManager& GetInstance()
    {
        static EventManager instance;
        return instance;
    }
    static EventManager& Get() { return GetInstance(); }

    // Post a message to be dispatched during the next Process() call.
    void AddMessage(const Message& msg)
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.push(msg);
    }

    // Immediately dispatch a message to registered listeners (no queue)
    void DispatchImmediate(const Message& msg)
    {
        std::vector<ListenerEntry> listenersCopy;
        {
            std::lock_guard<std::mutex> lock(m_listenersMutex);
            auto it = m_listeners.find(msg.msg_type);
            if (it != m_listeners.end())
                listenersCopy = it->second; // copy under lock
        }
        for (auto& entry : listenersCopy)
        {
            if (entry.second)
                entry.second(msg);
        }
    }

    // Process the queued messages and dispatch them to relevant listeners.
    void Process()
    {
        std::vector<Message> toDispatch;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_queue.empty())
            {
                toDispatch.push_back(m_queue.front());
                m_queue.pop();
            }
        }

        // Update ECS input-related components from messages before dispatching
        for (const auto& msg : toDispatch)
        {
            try
            {
                UpdateECSInputFromMessage(msg);
            }
            catch (...) { /* avoid throwing from the event loop */ }
        }

        for (const auto& msg : toDispatch)
        {
            DispatchImmediate(msg);
        }
    }

    // Register a generic callback for a specific event type. Owner is used to allow unregistering later.
    void Register(void* owner, EventType type, Listener callback)
    {
        if (!callback) return;
        std::lock_guard<std::mutex> lock(m_listenersMutex);
        auto& vec = m_listeners[type];
        vec.emplace_back(owner, std::move(callback));
    }

	// Generic template overload to register an object with an OnEvent method
    template<typename T>
    void Register(T* owner, EventType type)
    {
		// Check that T has a member function OnEvent(const Message&)
        static_assert(std::is_member_function_pointer<decltype(&T::OnEvent)>::value,
                      "Type T must have a member function OnEvent(const Message&)");
        
        if (!owner) return;
        Register(static_cast<void*>(owner), type, 
                 [owner](const Message& msg) { owner->OnEvent(msg); });
    }

    // Unregister a specific owner from a specific event type
    void Unregister(void* owner, EventType type)
    {
        std::lock_guard<std::mutex> lock(m_listenersMutex);
        auto it = m_listeners.find(type);
        if (it == m_listeners.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [owner](const ListenerEntry& e) { return e.first == owner; }), vec.end());
        if (vec.empty()) m_listeners.erase(it);
    }

    // Unregister owner from all event types
    void UnregisterAll(void* owner)
    {
        std::lock_guard<std::mutex> lock(m_listenersMutex);
        for (auto it = m_listeners.begin(); it != m_listeners.end(); )
        {
            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [owner](const ListenerEntry& e) { return e.first == owner; }), vec.end());
            if (vec.empty()) it = m_listeners.erase(it);
            else ++it;
        }
    }

private:
	std::string name;
    std::unordered_map<EventType, std::vector<ListenerEntry>> m_listeners;
    std::mutex m_listenersMutex;

    std::queue<Message> m_queue;
    std::mutex m_queueMutex;
};
