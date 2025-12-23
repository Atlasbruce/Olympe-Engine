/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.
Olympe Engine V2 is free software: you can redistribute it and/or modify

ECS Types purpose: Basic type definitions for the Entity-Component-System (ECS) architecture.

*/
#pragma once

#include <cstdint>
#include <bitset>
#include <vector>
#include <functional>

// --- 1. Entity (Identity) ---
// An Entity is just a unique ID.
using EntityID = std::uint64_t;
// /*DEPRECATED LIMITATION*/ const EntityID MAX_ENTITIES = 5000;
const EntityID INVALID_ENTITY_ID = 0; // Use 0 for invalid

// --- 2. Component (Data) ---
// Each component type gets a unique ID (its index in the bitset).
using ComponentTypeID = std::uint64_t;
const ComponentTypeID MAX_COMPONENTS = 32; // Limits the number of component types

// The Signature: indicates which components an Entity possesses.
using ComponentSignature = std::bitset<MAX_COMPONENTS>;

// Alias for the system update function
using SystemUpdateFn = std::function<void(float)>;

// --- 3. Pool Interface (for the World Registry) ---
// Virtual base class to store all component pools polymorphically
class IComponentPool
{
public:
    virtual ~IComponentPool() = default;
    // The virtual Remove method is essential for DestroyEntity()
    virtual void RemoveComponent(EntityID entity) = 0;
};

// --- Utility Functions for Type IDs ---
// Static counter to assign a unique ID to each new component type
inline ComponentTypeID GetComponentTypeID()
{
    static ComponentTypeID nextID = 1;
    return nextID++;
}

// Added templated GetComponentTypeID_Static to ensure a unique static typeID per component type
template<typename T>
ComponentTypeID GetComponentTypeID_Static()
{
    static ComponentTypeID typeID = GetComponentTypeID();
    return typeID;
}
