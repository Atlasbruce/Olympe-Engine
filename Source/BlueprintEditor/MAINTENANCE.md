# MAINTENANCE GUIDE: VisualScriptEditorPanel

**Version**: 1.0  
**Date**: Phase 5 Completion  
**Audience**: Maintainers, DevOps, QA, and support teams

---

## TABLE OF CONTENTS

1. [Common Issues & Solutions](#common-issues--solutions)
2. [Compilation Troubleshooting](#compilation-troubleshooting)
3. [Runtime Issues](#runtime-issues)
4. [Performance Optimization](#performance-optimization)
5. [Code Patterns to Avoid](#code-patterns-to-avoid)
6. [Refactoring Guidelines](#refactoring-guidelines)
7. [Testing Procedures](#testing-procedures)
8. [Logging & Diagnostics](#logging--diagnostics)
9. [Version Management](#version-management)
10. [Getting Help](#getting-help)

---

## COMMON ISSUES & SOLUTIONS

### Issue 1: Nodes Don't Appear on Canvas

**Symptoms**: 
- Added node, but it doesn't render
- Canvas remains empty
- No error messages

**Root Causes**:
1. Node not in m_editorNodes
2. RebuildLinks() not called after AddNode()
3. Canvas context not activated (ImNodes::EditorContextSet)

**Solution**:
```cpp
// In _Interaction.cpp, AddNode() must:
1. Create VSEditorNode
2. Add to m_editorNodes
3. Call m_needsPositionSync = true

// In _Canvas.cpp, RenderCanvas() must:
1. Call ImNodes::EditorContextSet(m_imnodesContext)
2. Call RebuildLinks() to recreate link list
```

**Verification**:
```cpp
// Debug: Check node count
SYSTEM_LOG << "Nodes: " << m_editorNodes.size() << "\n";

// Check if RebuildLinks was called
SYSTEM_LOG << "Links: " << m_editorLinks.size() << "\n";
```

---

### Issue 2: Links Not Connecting

**Symptoms**:
- Can't drag links between pins
- Links created but disappear
- "Invalid connection" error

**Root Causes**:
1. Pin UIDs calculated incorrectly
2. RebuildLinks() using wrong UID encoding
3. ConnectExec/ConnectData not pushing to undo stack

**Solution**:
```cpp
// Verify UID calculation in _Connections.cpp:
// Exec pins:     nodeID * 10000 + 0 (input)
//                nodeID * 10000 + 100 + index (output)
// Data pins:     nodeID * 10000 + 200 + index (input)
//                nodeID * 10000 + 300 + index (output)

// Check RebuildLinks() properly decodes these ranges
int srcNodeID = (srcAttrID / 10000);
int srcPinType = (srcAttrID % 10000) / 100;
```

**Verification**:
```cpp
// Add logging to ConnectExec/ConnectData:
SYSTEM_LOG << "Connecting: " << srcAttrID << " -> " << dstAttrID << "\n";
```

---

### Issue 3: Save/Load Fails

**Symptoms**:
- "Could not open file" error
- File exists but won't load
- Saved file corrupted

**Root Causes**:
1. File path is empty (m_currentFilePath)
2. JSON serialization failed silently
3. File permissions issue

**Solution**:
```cpp
// In _FileOps.cpp, check:
1. m_currentFilePath not empty
2. ValidateAndCleanBlackboardEntries() called
3. SyncPresetsFromRegistryToTemplate() called
4. File write succeeds (check return value)

// In SerializeAndWrite():
if (!outFile.is_open()) {
    SYSTEM_LOG << "Failed to open file: " << path << "\n";
    return false;
}
```

**Verification**:
```cpp
// Test save/load cycle:
SaveAs("test_blueprint.json");
LoadTemplate("test_blueprint.json");
assert(m_template.Name == "test_blueprint");
```

---

### Issue 4: Undo/Redo Not Working

**Symptoms**:
- Undo button doesn't work
- Graph state doesn't revert
- Multiple undos don't traverse history

**Root Causes**:
1. Command not pushed to undo stack
2. ICommand::Execute/Undo not implemented
3. SyncEditorNodesFromTemplate() not called after undo

**Solution**:
```cpp
// In _Interaction.cpp:
void VisualScriptEditorPanel::PerformUndo()
{
    m_undoStack.Undo();           // Step 1: Execute undo command
    SyncEditorNodesFromTemplate(); // Step 2: Sync editor state
    RebuildLinks();                // Step 3: Rebuild link list
    m_needsPositionSync = true;    // Step 4: Mark for position update
}
```

**Verification**:
```cpp
// Test undo/redo:
AddNode(TaskNodeType::Branch, 100, 100);  // Node added
PerformUndo();                             // Should disappear
PerformRedo();                             // Should reappear
assert(m_editorNodes.size() == 1);
```

---

### Issue 5: Verification Hangs

**Symptoms**:
- "Verify" button hangs
- UI freezes for > 5 seconds
- Verification never completes

**Root Causes**:
1. Infinite loop in verification logic
2. Circular graph reference
3. Performance issue with large graphs

**Solution**:
```cpp
// In _Verification.cpp, add safety checks:
void VisualScriptEditorPanel::RunVerification()
{
    const int MAX_CHECKS = 100000;  // Safety limit
    int checkCount = 0;

    for (const auto& node : m_template.Nodes) {
        if (++checkCount > MAX_CHECKS) {
            SYSTEM_LOG << "Verification safety limit hit\n";
            break;
        }
        // ... verification logic
    }
}
```

**Optimization**:
```cpp
// For large graphs, consider incremental verification
// or running on worker thread with progress callback
```

---

## COMPILATION TROUBLESHOOTING

### Issue: "Cannot find include file"

**Solution**:
```cmake
# In CMakeLists.txt, verify include directories:
target_include_directories(BlueprintEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}              # Current directory
    ${CMAKE_CURRENT_SOURCE_DIR}/..           # Parent (for third_party)
    ${CMAKE_SOURCE_DIR}/Source               # Source root
)
```

### Issue: "Unresolved external symbol"

**Solution**:
```cmake
# In CMakeLists.txt, verify linked libraries:
target_link_libraries(BlueprintEditor PRIVATE
    ImGui                    # ImGui library
    ImNodes                  # ImNodes library
    TaskGraphCore            # Graph definitions
    Phase24Components        # Phase 24 features
)
```

### Issue: "Conflicting C++ standard"

**Solution**:
```cmake
# In CMakeLists.txt, set standard explicitly:
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

---

## RUNTIME ISSUES

### Memory Leak: m_editorNodes Growing Unbounded

**Symptom**: Memory usage increases over time

**Fix**:
```cpp
// In _Interaction.cpp, RemoveNode() must fully clean up:
void VisualScriptEditorPanel::RemoveNode(int nodeID)
{
    // 1. Remove from m_editorNodes
    m_editorNodes.erase(
        std::remove_if(m_editorNodes.begin(), m_editorNodes.end(),
            [nodeID](const VSEditorNode& n) { return n.nodeID == nodeID; }),
        m_editorNodes.end()
    );

    // 2. Remove related links
    RemoveLinksForNode(nodeID);

    // 3. Remove from template
    m_template.Nodes.erase(
        std::remove_if(m_template.Nodes.begin(), m_template.Nodes.end(),
            [nodeID](const TaskNodeDefinition& n) { return n.NodeID == nodeID; }),
        m_template.Nodes.end()
    );
}
```

### Crash: Accessing Invalid Node Pointer

**Symptom**: Segmentation fault / access violation

**Prevention**:
```cpp
// Always validate pointers:
const TaskNodeDefinition* node = m_template.GetNode(nodeID);
if (!node) {
    SYSTEM_LOG << "Node " << nodeID << " not found\n";
    return;  // Don't dereference null pointer
}

// Or use bounds checking:
if (nodeID < 0 || nodeID >= m_editorNodes.size()) {
    return;  // Invalid index
}
```

### Performance: Slow Canvas Rendering

**Symptoms**: Low FPS, laggy interactions

**Optimization**:
```cpp
// In _Canvas.cpp, RebuildLinks() is expensive
// Cache results when possible:
if (m_linksNeedRebuild) {  // Only rebuild if dirty
    RebuildLinks();
    m_linksNeedRebuild = false;
}

// Limit node count for testing:
if (m_editorNodes.size() > 1000) {
    SYSTEM_LOG << "WARNING: Large graph (1000+ nodes)\n";
    // Consider culling off-screen nodes
}
```

---

## PERFORMANCE OPTIMIZATION

### Profiling Key Operations

```cpp
// Use high-resolution timer:
auto start = std::chrono::high_resolution_clock::now();

// Operation to profile
RebuildLinks();

auto end = std::chrono::high_resolution_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
SYSTEM_LOG << "RebuildLinks took: " << ms << " ms\n";
```

### Optimization Techniques

1. **Cache Frequently-Used Data**
   ```cpp
   // Instead of recalculating node positions every frame:
   std::map<int, std::pair<float, float>> m_nodePositionCache;

   if (m_nodePositionCache.empty()) {
       for (const auto& node : m_editorNodes) {
           m_nodePositionCache[node.nodeID] = 
               std::make_pair(node.posX, node.posY);
       }
   }
   ```

2. **Defer Expensive Operations**
   ```cpp
   // Don't rebuild links every frame, only when needed:
   if (m_linksChanged) {
       RebuildLinks();
       m_linksChanged = false;
   }
   ```

3. **Lazy Initialization**
   ```cpp
   // Create panels on-demand:
   if (!m_executionTestPanel) {
       m_executionTestPanel = std::make_unique<ExecutionTestPanel>();
   }
   ```

---

## CODE PATTERNS TO AVOID

### ❌ Anti-Pattern 1: Direct State Access Without Validation

```cpp
// BAD: No bounds checking
float x = m_editorNodes[100].posX;  // What if size < 101?

// GOOD: Validate before access
if (nodeID >= 0 && nodeID < m_editorNodes.size()) {
    float x = m_editorNodes[nodeID].posX;
}
```

### ❌ Anti-Pattern 2: Modifying While Iterating

```cpp
// BAD: Undefined behavior
for (auto& node : m_editorNodes) {
    if (node.def.Type == TaskNodeType::Delay) {
        m_editorNodes.erase(...);  // CRASH!
    }
}

// GOOD: Collect, then process
std::vector<int> toRemove;
for (const auto& node : m_editorNodes) {
    if (node.def.Type == TaskNodeType::Delay) {
        toRemove.push_back(node.nodeID);
    }
}
for (int id : toRemove) {
    RemoveNode(id);
}
```

### ❌ Anti-Pattern 3: Forgetting Dirty Flag

```cpp
// BAD: User can't save unsaved changes
m_template.Name = "New Name";
// m_dirty not set!

// GOOD: Always mark dirty after changes
m_template.Name = "New Name";
m_dirty = true;
```

### ❌ Anti-Pattern 4: Circular Dependencies

```cpp
// BAD: File A includes File B, File B includes File A
// VisualScriptEditorPanel_Rendering.cpp
#include "VisualScriptEditorPanel_Canvas.cpp"  // CIRCULAR!

// GOOD: Include only .h files, use forward declarations
class TaskNodeDefinition;  // Forward declare
void ProcessNode(const TaskNodeDefinition& node);
```

---

## REFACTORING GUIDELINES

### When to Create a New File

✅ **DO create new file if**:
- New domain with 200+ lines of code
- Distinct responsibility (e.g., a new panel)
- Multiple related methods that form a cohesive unit

❌ **DON'T create new file if**:
- Just 1-2 utility methods
- Temporary workaround
- Duplicate functionality already elsewhere

### When to Extend Existing File

✅ **DO extend if**:
- Closely related to existing domain
- Shares state/dependencies with existing methods
- < 100 additional lines

### Refactoring Safety

1. **Extract method**: Small, testable units
2. **Keep state access patterns**: Don't change how state is accessed
3. **Maintain headers**: Don't expose implementation details
4. **Update documentation**: Keep guides in sync with code

---

## TESTING PROCEDURES

### Unit Test Template

```cpp
// Test file: test_VisualScriptEditorPanel.cpp

TEST(VisualScriptEditorPanel, AddNodeCreatesNewNode)
{
    VisualScriptEditorPanel panel;
    panel.Initialize();

    int nodeID = panel.AddNode(TaskNodeType::Branch, 100, 100);

    EXPECT_GE(nodeID, 0);
    EXPECT_EQ(panel.GetNodeCount(), 1);
}

TEST(VisualScriptEditorPanel, SaveAndLoadPreservesGraph)
{
    VisualScriptEditorPanel panel1, panel2;
    panel1.Initialize();

    panel1.AddNode(TaskNodeType::Branch, 100, 100);
    panel1.SaveAs("test_graph.json");

    panel2.Initialize();
    panel2.LoadTemplate("test_graph.json");

    EXPECT_EQ(panel1.GetNodeCount(), panel2.GetNodeCount());
}
```

### Integration Test Checklist

- [ ] Load existing blueprint
- [ ] Add 5 nodes of different types
- [ ] Create links between nodes
- [ ] Save blueprint
- [ ] Close and reopen
- [ ] Verify all nodes/links restored
- [ ] Run verification
- [ ] Edit node properties
- [ ] Undo 3 times
- [ ] Redo 3 times
- [ ] Save final state

---

## LOGGING & DIAGNOSTICS

### Enable Detailed Logging

```cpp
// Set log level (if supported by SYSTEM_LOG):
SetLogLevel(LogLevel::Debug);

// Add diagnostic output:
#define DEBUG_LOG(msg) \
    SYSTEM_LOG << "[DEBUG] " << __FUNCTION__ << ": " << msg << "\n"

// Use in code:
DEBUG_LOG("Node " << nodeID << " added");
```

### Collect Diagnostic Information

When reporting issues, collect:

```
1. VisualScriptEditorPanel version: [from docs]
2. Operating system: [Windows/Linux/macOS]
3. C++ compiler: [MSVC/GCC/Clang]
4. Reproduction steps: [specific steps to reproduce]
5. Full error message: [complete error text]
6. Log file: [from application logs]
7. Sample blueprint: [if size < 1MB]
```

---

## VERSION MANAGEMENT

### Tracking Changes

Current version: **1.0** (Phase 5 Release)

```
1.0.0 - Initial refactored release
  - 10-file split from monolithic VisualScriptEditorPanel
  - Phase 24 Presets integration
  - Phase 24.3 Execution Testing (Phase 5 ready)

1.1.0 - Planned improvements
  - Performance optimizations
  - Additional node types
  - Enhanced verification
```

### Backward Compatibility

✅ **Guaranteed Compatible**:
- All public methods in header
- All method signatures unchanged
- All state members preserved

❌ **May Break**:
- If depending on internal file organization
- If relying on specific private method names
- If assuming specific compilation flags

---

## GETTING HELP

### Resources

1. **DEVELOPER_GUIDE.md** - How to add features
2. **PHASE24_INTEGRATION_GUIDE.md** - Phase 24 specifics
3. **PHASE3_VERIFICATION_REPORT.md** - Architecture details
4. **Original monolithic file** - VisualScriptEditorPanel.cpp (reference)

### Debugging Checklist

- [ ] Review error message carefully
- [ ] Check SYSTEM_LOG output
- [ ] Search for similar issues in this guide
- [ ] Review code in relevant .cpp file
- [ ] Add diagnostic logging
- [ ] Set breakpoint in suspicious area
- [ ] Test with minimal reproducible case

### When to Escalate

Escalate if:
- Multiple files affected simultaneously
- Crashes in link validation / serialization
- Performance degradation in large graphs
- Inconsistent behavior between platforms

---

**End of Maintenance Guide**

Last Updated: Phase 5 Release
Next Review: When adding major features or fixing critical issues
