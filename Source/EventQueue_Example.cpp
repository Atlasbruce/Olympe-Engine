/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file demonstrates usage of the EventQueue ECS system.
This is a standalone example showing how to use typed events.

*/

#include "ECS_Events.h"
#include "ECS_Components.h"
#include "ECS_Systems.h"
#include "World.h"
#include <iostream>
#include <cassert>

// Example: Simple event test
void TestEventQueue_Basic()
{
    std::cout << "=== EventQueue Basic Test ===" << std::endl;
    
    EventQueueSystem eventQueue;
    
    // Create and post an input button event
    InputButtonEvent btnEvt;
    btnEvt.sourceEntity = 1;
    btnEvt.deviceId = 0;
    btnEvt.buttonId = 2;
    btnEvt.pressed = true;
    btnEvt.timestamp = 1.5f;
    
    Event evt = Event::CreateInputButton(btnEvt);
    bool posted = eventQueue.PostEvent(ECSEventType::InputButton, evt);
    
    assert(posted && "Event should be posted successfully");
    assert(eventQueue.GetEventCount(ECSEventType::InputButton) == 1);
    
    // Consume the event
    int eventCount = 0;
    eventQueue.ConsumeEvents(ECSEventType::InputButton,
        [&](const Event& e) {
            eventCount++;
            assert(e.type == ECSEventType::InputButton);
            assert(e.data.inputButton.buttonId == 2);
            assert(e.data.inputButton.pressed == true);
            std::cout << "Consumed InputButton event: button=" << e.data.inputButton.buttonId 
                     << " pressed=" << e.data.inputButton.pressed << std::endl;
        });
    
    assert(eventCount == 1);
    assert(eventQueue.GetEventCount(ECSEventType::InputButton) == 0);
    
    std::cout << "Basic test PASSED!" << std::endl << std::endl;
}

// Example: Multiple events
void TestEventQueue_Multiple()
{
    std::cout << "=== EventQueue Multiple Events Test ===" << std::endl;
    
    EventQueueSystem eventQueue;
    
    // Post multiple collision events
    for (int i = 0; i < 5; i++)
    {
        CollisionEvent collision;
        collision.entityA = i;
        collision.entityB = i + 100;
        collision.started = true;
        collision.timestamp = i * 0.1f;
        
        Event evt = Event::CreateCollision(collision);
        eventQueue.PostEvent(ECSEventType::CollisionStarted, evt);
    }
    
    assert(eventQueue.GetEventCount(ECSEventType::CollisionStarted) == 5);
    
    // Consume all events in order
    int count = 0;
    eventQueue.ConsumeEvents(ECSEventType::CollisionStarted,
        [&](const Event& e) {
            assert(e.data.collision.entityA == count);
            count++;
        });
    
    assert(count == 5);
    std::cout << "Multiple events test PASSED!" << std::endl << std::endl;
}

// Example: Peek without consuming
void TestEventQueue_Peek()
{
    std::cout << "=== EventQueue Peek Test ===" << std::endl;
    
    EventQueueSystem eventQueue;
    
    // Post a trigger event
    TriggerEvent trigger;
    trigger.trigger = 42;
    trigger.other = 99;
    trigger.entered = true;
    
    Event evt = Event::CreateTrigger(trigger);
    eventQueue.PostEvent(ECSEventType::TriggerEntered, evt);
    
    // Peek at the event (should not remove it)
    int peekCount = 0;
    eventQueue.PeekEvents(ECSEventType::TriggerEntered,
        [&](const Event& e) {
            peekCount++;
            std::cout << "Peeked at event: trigger=" << e.data.trigger.trigger 
                     << " other=" << e.data.trigger.other << std::endl;
        });
    
    assert(peekCount == 1);
    assert(eventQueue.GetEventCount(ECSEventType::TriggerEntered) == 1); // Still in queue
    
    // Now consume it
    int consumeCount = 0;
    eventQueue.ConsumeEvents(ECSEventType::TriggerEntered,
        [&](const Event& e) { consumeCount++; });
    
    assert(consumeCount == 1);
    assert(eventQueue.GetEventCount(ECSEventType::TriggerEntered) == 0); // Removed
    
    std::cout << "Peek test PASSED!" << std::endl << std::endl;
}

// Example: Ring buffer overflow
void TestEventQueue_Overflow()
{
    std::cout << "=== EventQueue Overflow Test ===" << std::endl;
    
    EventQueueSystem eventQueue;
    
    // Fill the queue to capacity
    int successCount = 0;
    for (size_t i = 0; i < EventQueue_data::QUEUE_CAPACITY + 10; i++)
    {
        HealthChangedEvent health;
        health.entity = i;
        health.oldHealth = 100;
        health.newHealth = 90;
        health.delta = -10;
        
        Event evt = Event::CreateHealthChanged(health);
        if (eventQueue.PostEvent(ECSEventType::HealthChanged, evt))
        {
            successCount++;
        }
    }
    
    assert(successCount == EventQueue_data::QUEUE_CAPACITY);
    assert(eventQueue.GetEventCount(ECSEventType::HealthChanged) == EventQueue_data::QUEUE_CAPACITY);
    
    std::cout << "Posted " << successCount << " events (capacity: " 
             << EventQueue_data::QUEUE_CAPACITY << ")" << std::endl;
    
    // Clear and verify
    eventQueue.ClearEvents(ECSEventType::HealthChanged);
    assert(eventQueue.GetEventCount(ECSEventType::HealthChanged) == 0);
    
    std::cout << "Overflow test PASSED!" << std::endl << std::endl;
}

// Example: Entity-local queue
void TestEventQueue_EntityLocal()
{
    std::cout << "=== EventQueue Entity-Local Test ===" << std::endl;
    
    // Create an entity-local event queue
    EventQueue_data localQueue;
    
    // Post events to the local queue
    for (int i = 0; i < 3; i++)
    {
        ItemCollectedEvent item;
        item.collector = 1;
        item.item = i;
        item.itemType = "coin";
        
        Event evt = Event::CreateItemCollected(item);
        assert(localQueue.PushEvent(evt));
    }
    
    assert(localQueue.GetCount() == 3);
    
    // Pop events from the local queue
    Event evt;
    int popCount = 0;
    while (localQueue.PopEvent(evt))
    {
        assert(evt.type == ECSEventType::ItemCollected);
        std::cout << "Collected item " << evt.data.itemCollected.item << std::endl;
        popCount++;
    }
    
    assert(popCount == 3);
    assert(localQueue.IsEmpty());
    
    std::cout << "Entity-local test PASSED!" << std::endl << std::endl;
}

// Example: All event types
void TestEventQueue_AllTypes()
{
    std::cout << "=== EventQueue All Types Test ===" << std::endl;
    
    EventQueueSystem eventQueue;
    
    // Test each event type
    struct TestCase {
        ECSEventType type;
        const char* name;
    };
    
    TestCase testCases[] = {
        { ECSEventType::InputButton, "InputButton" },
        { ECSEventType::InputAxis, "InputAxis" },
        { ECSEventType::InputAction, "InputAction" },
        { ECSEventType::EntityJoined, "EntityJoined" },
        { ECSEventType::EntityLeft, "EntityLeft" },
        { ECSEventType::EntitySpawned, "EntitySpawned" },
        { ECSEventType::EntityDestroyed, "EntityDestroyed" },
        { ECSEventType::CollisionStarted, "CollisionStarted" },
        { ECSEventType::CollisionEnded, "CollisionEnded" },
        { ECSEventType::TriggerEntered, "TriggerEntered" },
        { ECSEventType::TriggerExited, "TriggerExited" },
        { ECSEventType::HealthChanged, "HealthChanged" },
        { ECSEventType::ItemCollected, "ItemCollected" },
        { ECSEventType::LevelLoaded, "LevelLoaded" },
        { ECSEventType::LevelUnloaded, "LevelUnloaded" },
        { ECSEventType::GamePaused, "GamePaused" },
        { ECSEventType::GameResumed, "GameResumed" }
    };
    
    for (const auto& test : testCases)
    {
        // Create a generic event for each type
        Event evt;
        evt.type = test.type;
        
        bool posted = eventQueue.PostEvent(test.type, evt);
        assert(posted);
        
        std::cout << "Posted " << test.name << " event" << std::endl;
    }
    
    // Clear all queues
    eventQueue.ClearAllEvents();
    
    std::cout << "All types test PASSED!" << std::endl << std::endl;
}

// Main test runner
int main()
{
    std::cout << "======================================" << std::endl;
    std::cout << "EventQueue ECS System - Test Suite" << std::endl;
    std::cout << "======================================" << std::endl << std::endl;
    
    try
    {
        TestEventQueue_Basic();
        TestEventQueue_Multiple();
        TestEventQueue_Peek();
        TestEventQueue_Overflow();
        TestEventQueue_EntityLocal();
        TestEventQueue_AllTypes();
        
        std::cout << "======================================" << std::endl;
        std::cout << "All tests PASSED! ✓" << std::endl;
        std::cout << "======================================" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
