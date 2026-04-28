# PHASE 45 - STEP 1: LEGACY SAVE FLOW ANALYSIS

## 🎯 Objective
Document exactly how the legacy NodeGraphPanel Save/SaveAs works so we can replicate it in the framework.

---

## 🔍 LEGACY SAVE FLOW TRACE

### **1. USER INTERACTION (NodeGraphPanel::RenderContent)**

#### **1a. Save Button (Line 201-225)**
```cpp
if (ImGui::Button("Save"))
{
    // Step 1: Validate graph
    std::string validationError;
    if (!activeGraph->ValidateGraph(validationError))
    {
        ImGui::OpenPopup("ValidationError");
        return;
    }
    
    // Step 2: Sync ImNodes positions to internal state
    int graphId = NodeGraphManager::Get().GetActiveGraphId();
    SyncNodePositionsFromImNodes(graphId);
    
    // Step 3: Get existing filepath from graph
    const std::string& filepath = activeGraph->GetFilepath();
    
    // Step 4: Call backend to save
    if (NodeGraphManager::Get().SaveGraph(graphId, filepath))
    {
        std::cout << "[NodeGraphPanel] Saved graph to: " << filepath << std::endl;
    }
    else
    {
        std::cout << "[NodeGraphPanel] Failed to save graph!" << std::endl;
    }
}
```

**Key Points:**
- ✅ Validates graph before save
- ✅ Syncs ImNodes visual state with internal state
- ✅ Uses existing filepath from graph metadata
- ✅ Delegates serialization to backend
- ✅ Logs success/failure

**Preconditions:**
- filepath must be set (button disabled if empty)
- Graph must be valid

---

#### **1b. Save As Button (Line 238-330)**
```cpp
if (ImGui::Button("Save As..."))
{
    ImGui::OpenPopup("SaveAsPopup");
}

if (ImGui::BeginPopup("SaveAsPopup"))
{
    // Popup initialized once
    if (!saveAsPopupOpen)
    {
        filepathBuffer[0] = '\0';
        saveAsPopupOpen = true;
    }
    
    ImGui::Text("Save graph as:");
    ImGui::InputText("Filepath", filepathBuffer, sizeof(filepathBuffer));
    
    if (ImGui::Button("Save", ImVec2(120, 0)))
    {
        std::string filepath(filepathBuffer);
        if (!filepath.empty())
        {
            // Validate graph
            std::string validationError;
            if (!activeGraph->ValidateGraph(validationError))
            {
                saveAsPopupOpen = false;
                ImGui::CloseCurrentPopup();
                ImGui::OpenPopup("ValidationError");
                return;
            }
            
            // Ensure .json extension
            if (filepath.size() < 5 || filepath.substr(filepath.size() - 5) != ".json")
                filepath += ".json";
            
            // Sync and save with NEW filepath
            int graphId = NodeGraphManager::Get().GetActiveGraphId();
            SyncNodePositionsFromImNodes(graphId);
            if (NodeGraphManager::Get().SaveGraph(graphId, filepath))
            {
                std::cout << "[NodeGraphPanel] Saved graph as: " << filepath << std::endl;
                saveAsPopupOpen = false;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                std::cout << "[NodeGraphPanel] Failed to save graph!" << std::endl;
            }
        }
    }
    
    if (ImGui::Button("Cancel"))
    {
        saveAsPopupOpen = false;
        ImGui::CloseCurrentPopup();
    }
}
```

**Key Points:**
- ✅ Opens modal popup for filepath input
- ✅ Validates input (not empty)
- ✅ Auto-adds .json extension if missing
- ✅ Validates graph before saving
- ✅ Syncs ImNodes state
- ✅ Saves with NEW filepath
- ✅ Clears popup state on success

---

### **2. BACKEND SAVE (BTNodeGraphManager::SaveGraph)**

#### **Implementation (Lines 930-953)**
```cpp
bool NodeGraphManager::SaveGraph(int graphId, const std::string& filepath)
{
    // Line 930-937: Update metadata timestamp
    auto time = std::time(nullptr);
    std::stringstream ss;
    #ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
    #else
        std::tm timeinfo;
        localtime_r(&time, &timeinfo);
        ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
    #endif
    graph->editorMetadata.lastModified = ss.str();
    
    // Line 938: SERIALIZE to JSON
    json j = graph->ToJson();
    
    // Line 940-945: WRITE to file
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;
    file << j.dump(2);
    file.close();
    
    // Line 948-949: UPDATE state
    graph->SetFilepath(filepath);
    graph->ClearDirty();  // ← CRITICAL: Clear dirty flag on success
    
    // Line 951: Log
    std::cout << "[NodeGraphManager] Saved graph " << graphId 
              << " to " << filepath << "\n";
    return true;
}
```

**Critical Operations:**
1. ✅ Update `editorMetadata.lastModified` timestamp
2. ✅ Call `graph->ToJson()` for serialization
3. ✅ Write JSON to file with pretty-print (indent=2)
4. ✅ Update graph's filepath
5. ✅ **Clear dirty flag** - MUST NOT FORGET
6. ✅ Return true on success

**Error Handling:**
- Returns false if file cannot be opened
- No exception handling needed (user must retry)

---

### **3. STATE MANAGEMENT**

#### **Dirty Flag Lifecycle**
- **Set to dirty**: When graph is modified (`graph->MarkDirty()`)
- **Checked before UI**: `activeGraph->IsDirty()` shows "*" indicator
- **Cleared after save**: `graph->ClearDirty()` in SaveGraph()
- **Button enabled if**: `activeGraph->GetFilepath()` is not empty

#### **Filepath Lifecycle**
- **Initialized**: On LoadGraph() - line 1122 `graph->SetFilepath(filepath)`
- **Updated on Save**: In SaveGraph() - line 948 `graph->SetFilepath(filepath)`
- **Used in Save**: Line 215 `const std::string& filepath = activeGraph->GetFilepath()`

---

## 📊 EXTRACTED SAVE PATTERN

### **Required Operations for Framework**
```
OnSaveClicked():
  1. Get active document
  2. If no filepath → call OnSaveAsClicked() instead
  3. Validate graph
  4. Sync ImNodes state (if applicable)
  5. Call document->Save(filepath)
  6. Log success/failure

OnSaveAsClicked():
  1. Open modal for filepath input
  2. Wait for user input
  3. Validate input
  4. Ensure .json extension
  5. Validate graph
  6. Call document->Save(newFilepath)
  7. Close modal on success

document->Save(filepath):
  1. Update metadata timestamp
  2. Serialize to JSON via graph->ToJson()
  3. Write to file
  4. Update internal filepath
  5. Clear dirty flag
  6. Return true/false
```

---

## 🔑 KEY INTEGRATION POINTS FOR FRAMEWORK

### **1. Dirty Flag Management**
- Must be cleared AFTER successful save
- Must be set WHEN graph is modified
- Framework must check before save

### **2. Filepath Tracking**
- Save button disabled if filepath empty
- SaveAs must update filepath after successful save
- File path stored in IGraphDocument

### **3. Validation**
- Graph must be valid before save
- Validation errors shown in modal
- User cannot save invalid graph

### **4. ImNodes Sync** (for BehaviorTree only)
- Must call `SyncNodePositionsFromImNodes()` before save
- Ensures visual positions match internal state

### **5. Extension Handling**
- SaveAs auto-adds `.json` if missing
- Framework should handle this

### **6. Serialization Backend**
- Uses existing `graph->ToJson()` method
- Already handles all schema details
- Framework delegates to this

---

## ✅ DELIVERABLES FROM THIS ANALYSIS

**What we know works:**
- ✅ NodeGraphManager::SaveGraph() - Proven serialization backend
- ✅ Dirty flag clearing mechanism
- ✅ SaveAs popup flow
- ✅ Validation before save
- ✅ ImNodes state synchronization

**What framework must implement:**
- ✅ IGraphDocument::Save() methods in each document type
- ✅ Delegate to existing backend (reuse not rewrite)
- ✅ CanvasToolbarRenderer button wiring
- ✅ Modal flow for SaveAs

**NO need to reimplement:**
- ❌ JSON serialization (use graph->ToJson())
- ❌ File I/O (use std::ofstream)
- ❌ Validation logic (use graph->ValidateGraph())

---

## 🚀 NEXT STEP (Step 2)

Extract the reusable pattern into adapter implementation in BehaviorTreeGraphDocument::Save().

Delegation pattern: Framework → IGraphDocument → NodeGraphManager (proven backend)
