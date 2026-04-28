# PHASE 45 - STEP 2: REUSABLE SAVE LOGIC EXTRACTION

## 🎯 Objective
Extract the core serialization pattern from BTNodeGraphManager::SaveGraph() that can be reused by the framework across all document types.

---

## 📋 EXTRACTED SAVE BACKEND PATTERN

### **Core Serialization Pipeline**

```cpp
/**
 * PATTERN: Universal Save Backend for any graph type
 * 
 * Works with ANY graph that implements:
 * - graph->ToJson() : Serializes to nlohmann::json
 * - graph->SetFilepath(path) : Stores filepath
 * - graph->ClearDirty() : Marks as saved
 * - graph->editorMetadata.lastModified : Timestamp field
 */

bool SaveGraphBackend(
    const std::string& filepath,
    NodeGraph* graph  // Generic graph pointer
)
{
    // Step 1: Update metadata
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    
    #ifdef _MSC_VER
        std::tm timeinfo;
        localtime_s(&timeinfo, &time);
        ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
    #else
        std::tm timeinfo;
        localtime_r(&time, &timeinfo);
        ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
    #endif
    
    graph->editorMetadata.lastModified = ss.str();
    
    // Step 2: Serialize
    json j = graph->ToJson();
    
    // Step 3: Write to file
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        SYSTEM_LOG << "[SaveGraph] ERROR: Cannot open file: " << filepath << "\n";
        return false;
    }
    
    file << j.dump(2);
    file.close();
    
    // Step 4: Update state
    graph->SetFilepath(filepath);
    graph->ClearDirty();  // ← CRITICAL
    
    SYSTEM_LOG << "[SaveGraph] SUCCESS: Saved to " << filepath << "\n";
    return true;
}
```

---

## 🔗 INTEGRATION POINTS

### **Where it's called from (Legacy)**
```cpp
// NodeGraphPanel::RenderContent() line 201 or 238
if (ImGui::Button("Save"))
{
    // ... validation and sync ...
    int graphId = NodeGraphManager::Get().GetActiveGraphId();
    SyncNodePositionsFromImNodes(graphId);
    
    const std::string& filepath = activeGraph->GetFilepath();
    
    // CALLS THIS:
    if (NodeGraphManager::Get().SaveGraph(graphId, filepath))
    {
        // Success
    }
}
```

### **Framework must replicate**
```cpp
// BehaviorTreeGraphDocument::Save(filepath) 
// NEW - Framework delegation point
{
    // Get active graph from NodeGraphManager
    int graphId = NodeGraphManager::Get().GetActiveGraphId();
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
    
    // Sync ImNodes state
    SyncNodePositionsFromImNodes(graphId);
    
    // Call backend pattern (reuse, not rewrite)
    bool result = SaveGraphBackend(filepath, graph);
    
    return result;
}
```

---

## 📊 CRITICAL IMPLEMENTATION REQUIREMENTS

### **1. Dirty Flag Management**
```cpp
// BEFORE save
if (graph->IsDirty())
    ImGui::Text("*");  // Show unsaved indicator

// AFTER successful save
graph->ClearDirty();   // ← MUST HAPPEN

// Test: Verify "*" disappears after save
```

### **2. Filepath Persistence**
```cpp
// Store filepath so Save button works next time
graph->SetFilepath(filepath);

// Later:
const std::string& path = graph->GetFilepath();
if (path.empty())
    // Disable Save button, show SaveAs tooltip
```

### **3. Validation Gate**
```cpp
// BEFORE attempting save
std::string validationError;
if (!graph->ValidateGraph(validationError))
{
    SYSTEM_LOG << "[Save] Validation failed: " << validationError << "\n";
    ShowModal("ValidationError", validationError);
    return false;  // DO NOT SAVE
}

// AFTER validation passes
// ... proceed to save ...
```

### **4. File Extension Handling**
```cpp
// SaveAs must ensure .json extension
std::string filepath(userInput);
if (filepath.size() < 5 || filepath.substr(filepath.size() - 5) != ".json")
{
    filepath += ".json";
}

// Then proceed to save
SaveGraphBackend(filepath, graph);
```

### **5. ImNodes State Sync** (BehaviorTree only)
```cpp
// BEFORE save (for NodeGraph-based editors only)
int graphId = NodeGraphManager::Get().GetActiveGraphId();
SyncNodePositionsFromImNodes(graphId);

// This ensures visual node positions are persisted
```

---

## 🏗️ FRAMEWORK ADAPTER PATTERN

### **BehaviorTreeGraphDocument::Save() Implementation Template**
```cpp
bool BehaviorTreeGraphDocument::Save(const std::string& filepath)
{
    if (filepath.empty())
    {
        SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: Empty filepath\n";
        return false;
    }
    
    // Get active graph from manager
    int graphId = NodeGraphManager::Get().GetActiveGraphId();
    if (graphId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: No active graph\n";
        return false;
    }
    
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
    if (!graph)
    {
        SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: Graph not found\n";
        return false;
    }
    
    // Validate graph
    std::string validationError;
    if (!graph->ValidateGraph(validationError))
    {
        m_lastError = "Validation failed: " + validationError;
        SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] " << m_lastError << "\n";
        return false;
    }
    
    // Sync ImNodes visual state to internal state
    if (m_renderer)
    {
        // Call renderer's sync method
        m_renderer->SyncNodePositionsFromImNodes(graphId);
    }
    
    // DELEGATE TO PROVEN BACKEND
    // Use NodeGraphManager directly (existing, working method)
    return NodeGraphManager::Get().SaveGraph(graphId, filepath);
    
    // Backend handles:
    // - Metadata timestamp update
    // - JSON serialization (graph->ToJson())
    // - File I/O
    // - Filepath update
    // - Dirty flag clearing
}
```

---

## ✅ KEY OBSERVATIONS

### **What already works perfectly**
- ✅ `NodeGraphManager::SaveGraph(graphId, filepath)` - Proven, tested, reliable
- ✅ `graph->ToJson()` - Serialization already handles all schema versions
- ✅ `graph->ClearDirty()` - State management works
- ✅ File I/O with pretty-print JSON

### **What framework must add**
- ✅ IGraphDocument::Save() interface methods
- ✅ Adapter pattern in document implementations
- ✅ Button wiring in CanvasToolbarRenderer
- ✅ Modal for SaveAs
- ✅ Error handling and user feedback

### **What NOT to change**
- ❌ DON'T rewrite NodeGraphManager::SaveGraph()
- ❌ DON'T change ToJson() serialization
- ❌ DON'T modify dirty flag logic
- ❌ DON'T touch file I/O implementation

---

## 📌 NO REGRESSION COMMITMENT

**This approach guarantees safety because:**
1. We REUSE proven backend methods (not replace them)
2. We ADD adapter layer (not modify existing code)
3. Legacy can coexist during transition
4. Can rollback to legacy anytime if needed

---

## 🚀 NEXT STEP (Step 3)

Implement `BehaviorTreeGraphDocument::Save()` using the adapter pattern above.

This method will be the key bridge between:
- Framework UI (CanvasToolbarRenderer buttons)
- Legacy Backend (NodeGraphManager::SaveGraph())
