#pragma once

#include "message.h"
#include <vector>
#include <array>
#include <mutex>

// EventQueue: ECS-first double-buffered event queue with domain routing
// Events are written to writeEvents during frame N and become readable in readEvents during frame N+1
// after BeginFrame() swaps the buffers.
class EventQueue
{
public:
    EventQueue()
    {
        // Initialize domain index vectors for read/write buffers
        for (int i = 0; i < static_cast<int>(EventDomain::All); ++i)
        {
            m_readDomainIdx[i].reserve(64);
            m_writeDomainIdx[i].reserve(64);
        }
        m_readEvents.reserve(256);
        m_writeEvents.reserve(256);
    }

    ~EventQueue() = default;

    // Singleton accessors
    static EventQueue& GetInstance()
    {
        static EventQueue instance;
        return instance;
    }
    static EventQueue& Get() { return GetInstance(); }

    // Push an event to the write buffer (called during frame N, visible frame N+1)
    void Push(const Message& msg)
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        
        size_t idx = m_writeEvents.size();
        m_writeEvents.push_back(msg);
        
        // Route to domain-specific index lists
        int domainIdx = static_cast<int>(msg.domain);
        if (domainIdx >= 0 && domainIdx < static_cast<int>(EventDomain::All))
        {
            m_writeDomainIdx[domainIdx].push_back(idx);
        }
        
        // Also route to "All" domain for systems that need all events
        m_writeDomainIdx[static_cast<int>(EventDomain::All)].push_back(idx);
    }

    // Swap read/write buffers - called once per frame at the start
    void BeginFrame()
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        
        // Swap event vectors
        m_readEvents.swap(m_writeEvents);
        m_writeEvents.clear();
        
        // Swap domain index vectors
        for (int i = 0; i <= static_cast<int>(EventDomain::All); ++i)
        {
            m_readDomainIdx[i].swap(m_writeDomainIdx[i]);
            m_writeDomainIdx[i].clear();
        }
    }

    // Get all events from the read buffer (frame N-1 events)
    const std::vector<Message>& GetEvents() const
    {
        return m_readEvents;
    }

    // Get events for a specific domain from the read buffer
    // Returns indices into the main event vector
    const std::vector<size_t>& GetDomainIndices(EventDomain domain) const
    {
        int idx = static_cast<int>(domain);
        if (idx >= 0 && idx <= static_cast<int>(EventDomain::All))
        {
            return m_readDomainIdx[idx];
        }
        static std::vector<size_t> empty;
        return empty;
    }

    // Helper: iterate domain events with callback
    template<typename Func>
    void ForEachDomainEvent(EventDomain domain, Func&& callback) const
    {
        const auto& indices = GetDomainIndices(domain);
        for (size_t idx : indices)
        {
            if (idx < m_readEvents.size())
            {
                callback(m_readEvents[idx]);
            }
        }
    }

private:
    // Double buffer: read buffer contains events from previous frame (N-1)
    // write buffer accumulates events for current frame (N)
    std::vector<Message> m_readEvents;
    std::vector<Message> m_writeEvents;
    
    // Domain routing: indices into the event vectors by domain
    // Index corresponds to EventDomain enum value
    std::array<std::vector<size_t>, static_cast<int>(EventDomain::All) + 1> m_readDomainIdx;
    std::array<std::vector<size_t>, static_cast<int>(EventDomain::All) + 1> m_writeDomainIdx;
    
    std::mutex m_writeMutex; // Protect write operations
};
