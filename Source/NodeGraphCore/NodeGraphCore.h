/**
 * @file NodeGraphCore.h
 * @brief Core data structures for generic node graph system
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Provides generic data structures for node-based graphs that can be reused
 * across multiple editors (BehaviorTree, HFSM, Animation, Blueprint).
 * All code strictly compliant with C++14 standard.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// ID Types
// ============================================================================

struct GraphId {
    uint32_t value = 0;
    
    bool operator==(const GraphId& other) const { return value == other.value; }
    bool operator!=(const GraphId& other) const { return value != other.value; }
    bool operator<(const GraphId& other) const { return value < other.value; }
};

struct NodeId {
    uint32_t value = 0;
    
    bool operator==(const NodeId& other) const { return value == other.value; }
    bool operator!=(const NodeId& other) const { return value != other.value; }
    bool operator<(const NodeId& other) const { return value < other.value; }
};

struct PinId {
    uint32_t value = 0;
    
    bool operator==(const PinId& other) const { return value == other.value; }
    bool operator!=(const PinId& other) const { return value != other.value; }
    bool operator<(const PinId& other) const { return value < other.value; }
};

struct LinkId {
    uint32_t value = 0;
    
    bool operator==(const LinkId& other) const { return value == other.value; }
    bool operator!=(const LinkId& other) const { return value != other.value; }
    bool operator<(const LinkId& other) const { return value < other.value; }
};

// ============================================================================
// Basic Structures
// ============================================================================

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
    
    Vector2() = default;
    Vector2(float inX, float inY) : x(inX), y(inY) {}
};

// ============================================================================
// Node Data
// ============================================================================

struct NodeData {
    NodeId id;
    std::string type;
    std::string name;
    Vector2 position;
    std::map<std::string, std::string> parameters;
    std::vector<NodeId> children;
    NodeId decoratorChild;
};

// ============================================================================
// Pin Data
// ============================================================================

struct PinData {
    PinId id;
    NodeId nodeId;
    std::string type;
    std::string name;
};

// ============================================================================
// Link Data
// ============================================================================

struct LinkData {
    LinkId id;
    PinId fromPin;
    PinId toPin;
};

// ============================================================================
// Editor State
// ============================================================================

struct EditorState {
    float zoom = 1.0f;
    Vector2 scrollOffset;
    std::vector<NodeId> selectedNodes;
    std::string layoutDirection = "TopToBottom";
};

} // namespace NodeGraph
} // namespace Olympe
