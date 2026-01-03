// Minimal embedded version of IM Nodes (stub) to allow building the editor without full dependency.
// This header provides a tiny API surface similar to imnodes so the editor can be developed
// further. Replace with the real imnodes library when available.

#pragma once

namespace imnodes
{
    inline void CreateContext() {}
    inline void DestroyContext(void*) {}
    inline void BeginNode(int id) {}
    inline void EndNode() {}
    inline void BeginNodeTitleBar() {}
    inline void EndNodeTitleBar() {}
    inline void BeginInputAttribute(int id) {}
    inline void EndInputAttribute() {}
    inline void BeginOutputAttribute(int id) {}
    inline void EndOutputAttribute() {}
    inline void NodeEditor() {}
}

// Optional helper to check availability
#define IMNODES_AVAILABLE 1
