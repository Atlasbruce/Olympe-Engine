# Phase 44.2.1 - Complete Legacy SaveAs Workflow Trace

## EXECUTIVE SUMMARY

The legacy toolbar buttons (working) call a **complete 7-step save workflow** that the framework toolbar buttons do NOT call:

```
LEGACY: Button → flag → modal → filename → SaveAs() → serialize → file write
FRAMEWORK: Button → flag → modal rendered (but then nothing happens)
```

## COMPLETE CALL CHAIN - LEGACY SYSTEM

### Step 1: User Clicks "Save As" Button
**File**: `VisualScriptEditorPanel - backup.cpp`  
**Lines**: 2134-2137
```cpp
if (ImGui::Button("Save As"))
{
    m_showSaveAsDialog = true;  // Just sets a flag
}
```
**What happens**: Simple flag set. Nothing else in the button handler.

---

### Step 2: Main Render Loop Detects Flag
**File**: `VisualScriptEditorPanel_RenderingCore.cpp`  
**Lines**: 362-408
**Function**: `RenderSaveAsDialog()`

```cpp
void VisualScriptEditorPanel::RenderSaveAsDialog()
{
    // LEGACY FALLBACK (when m_framework is null)
    if (m_showSaveAsDialog)
    {
        // Prepare suggested filename (remove extension)
        std::string suggestedName = m_currentPath.empty() 
                                    ? "graph" 
                                    : m_currentPath.substr(...);
        size_t dotPos = suggestedName.rfind('.');
        if (dotPos != std::string::npos)
            suggestedName = suggestedName.substr(0, dotPos);
        
        // OPEN THE MODAL
        dm.OpenSaveFilePickerModal(Olympe::SaveFileType::Blueprint, 
                                   "Gamedata/TaskGraph", 
                                   suggestedName);
        m_showSaveAsDialog = false;  // Only open once
    }
    
    // RENDER THE MODAL EVERY FRAME
    dm.RenderSaveFilePickerModal();
    
    // HANDLE COMPLETION
    if (dm.IsSaveFilePickerModalOpen() == false) {
        std::string selectedFile = dm.GetSelectedSaveFile();
        if (!selectedFile.empty()) {
            if (SaveAs(selectedFile)) {      // *** CALLS SaveAs() ***
                m_currentPath = selectedFile; // *** UPDATES PATH ***
            }
        }
    }
}
```

**Key points**:
1. Opens modal with suggested filename
2. Renders modal every frame
3. Checks if modal closed (user clicked Save or Cancel)
4. **IF user clicked Save**: calls `SaveAs(selectedFile)`

---

### Step 3: SaveAs() Method
**File**: `VisualScriptEditorPanel_FileOperations.cpp`  
**Lines**: 241-330
```cpp
bool VisualScriptEditorPanel::SaveAs(const std::string& path)
{
    // ... validation checks ...
    
    // SYNC DATA
    ResetViewportBeforeSave();
    CommitPendingBlackboardEdits();
    ValidateAndCleanBlackboardEntries();
    SyncPresetsFromRegistryToTemplate();
    
    if (m_entityBlackboard)
        m_template.GlobalVariableValues = m_entityBlackboard->ExportGlobalsToJson();
    
    // SYNC POSITIONS
    SyncNodePositionsFromImNodes();
    
    // SERIALIZE & WRITE
    bool ok = SerializeAndWrite(path);  // *** KEY CALL ***
    
    // RESTORE STATE
    AfterSave();
    
    return ok;
}
```

**What this does**:
- Synchronizes all editor data (blackboard, presets, global variables)
- Syncs node positions from ImNodes
- **Calls SerializeAndWrite() to actually save**

---

### Step 4: SerializeAndWrite() - The Serialization Engine
**File**: `VisualScriptEditorPanel_FileOperations.cpp`  
**Lines**: 630-994
```cpp
bool VisualScriptEditorPanel::SerializeAndWrite(const std::string& path)
{
    // BUILD JSON STRUCTURE
    json root;
    root["schema_version"] = 4;
    root["blueprintType"] = "VisualScript";
    root["name"] = m_template.Name;
    root["description"] = m_template.Description;
    root["version"] = m_template.Version;
    
    // SERIALIZE NODES
    json nodesArray = json::array();
    for (const auto& node : m_template.Nodes)
    {
        // ... serialize each node to JSON ...
    }
    root["data"]["nodes"] = nodesArray;
    
    // SERIALIZE CONNECTIONS
    json connectionsArray = json::array();
    for (const auto& conn : m_template.Connections)
    {
        // ... serialize each connection to JSON ...
    }
    root["data"]["connections"] = connectionsArray;
    
    // SERIALIZE BLACKBOARDS
    json blackboardData = json::object();
    for (const auto& var : m_template.Blackboard)
    {
        // ... serialize blackboard variables ...
    }
    root["data"]["blackboard"] = blackboardData;
    
    // SERIALIZE PRESETS (Phase 24)
    json presetsArray = json::array();
    for (const auto& preset : m_template.Presets)
    {
        presetsArray.push_back(preset.ToJson());
    }
    root["presets"] = presetsArray;
    
    // WRITE TO DISK
    std::ofstream ofs(path);
    if (!ofs.is_open())
        return false;
    
    ofs << root.dump(2);  // Pretty-print JSON
    ofs.close();
    
    m_dirty = false;  // *** CLEAR DIRTY FLAG ***
    return true;
}
```

**Critical operations**:
1. Builds complete JSON from m_template
2. Serializes all nodes, connections, blackboards, presets
3. **Writes JSON to disk**
4. **Sets m_dirty = false**

---

### Step 5: Update UI After Save
**File**: `VisualScriptEditorPanel_RenderingCore.cpp` (legacy fallback)  
**Lines**: 395-401
```cpp
if (!selectedFile.empty()) {
    if (SaveAs(selectedFile)) {
        m_currentPath = selectedFile;  // *** UPDATE PATH ***
        // Tab title will show m_currentPath next render
    }
}
```

**Also in toolbar render** (`VisualScriptEditorPanel - backup.cpp`, lines 2114-2117):
```cpp
const char* title = m_currentPath.empty()
                    ? "Untitled VS Graph"
                    : m_currentPath.c_str();
ImGui::TextDisabled("%s%s", title, m_dirty ? " *" : "");
```

**What happens**:
- Title shows m_currentPath (no longer "Untitled-1")
- Dirty flag `*` disappears (m_dirty = false)
- Tab display name updates automatically

---

## COMPLETE CALL CHAIN - FRAMEWORK SYSTEM (INCOMPLETE)

### What Framework DOES
**File**: `Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp`  
**Lines**: 318-346
```cpp
void CanvasToolbarRenderer::OnSaveAsClicked()
{
    SYSTEM_LOG << "[CanvasToolbarRenderer] SaveAs button clicked\n";
    m_showSaveAsModal = true;  // Just sets a flag
}

void CanvasToolbarRenderer::RenderModals()
{
    // ... open and render modal ...
    if (m_showSaveAsModal)
    {
        // Modal opens and renders
        CanvasModalRenderer::Get().RenderSaveFilePickerModal();
        m_showSaveAsModal = false;
    }
}
```

### What Framework DOESN'T DO
**MISSING**:
- ❌ No check if modal closed
- ❌ No call to `document.SaveAs()`
- ❌ No serialization
- ❌ No file write
- ❌ No m_dirty flag clearing
- ❌ No m_currentPath update
- ❌ No UI update

---

## ROOT CAUSE

Framework toolbar only manages **modal UI rendering**, it does NOT:

```
Button Click → Flag Set → Modal Opened/Rendered ← STOPS HERE

MISSING:
Modal Closed → User confirmed → Call SaveAs() → Serialize → Write → Update UI
```

---

## SOLUTION REQUIRED

For each editor type (VisualScript, BehaviorTree, EntityPrefab), implement:

1. **In CanvasToolbarRenderer::RenderModals()**:
   - Check if modal closed (was user confirmed?)
   - Get selected filename from modal
   - Call `m_document->SaveAs(filename)`
   - Update `m_document->m_currentPath` (or equivalent)
   - Clear dirty flag

2. **In IGraphDocument interface**:
   - Implement `SaveAs(const std::string& path)` for each graph type
   - Implement getters for current path, dirty flag
   - Implement serialization methods

3. **In each renderer** (VisualScriptRenderer, BehaviorTreeRenderer, EntityPrefabRenderer):
   - Ensure document interface methods delegate to actual implementation
   - Ensure UI updates after save (tab title, dirty flag display)

---

## SERIALIZATION METHODS BY EDITOR TYPE

### VisualScript
- **Document**: `TaskGraphTemplate`
- **Serialization**: `VisualScriptEditorPanel::SerializeAndWrite(path)`
- **Dirty flag**: `m_dirty`
- **Current path**: `m_currentPath`

### BehaviorTree  
- **Document**: `BehaviorTreeGraphDocument` (adapter wraps JSON)
- **Serialization**: Needs to be ported from legacy BT system
- **Dirty flag**: `m_isDirty` (or needs to be added)
- **Current path**: `m_filePath`

### EntityPrefab
- **Document**: `EntityPrefabGraphDocument` (adapter for entity prefab JSON)
- **Serialization**: `EntityPrefabGraphDocument::SaveToFile(path)`
- **Dirty flag**: `m_isDirty`
- **Current path**: `m_graphPath`

---

## NEXT STEPS

1. **Port legacy SaveAs workflow to CanvasToolbarRenderer**
2. **Implement full SaveAs() in BehaviorTreeGraphDocument**
3. **Verify all three editors complete cycle: Save → Serialize → Write → UI Update**
4. **Remove duplicate legacy toolbar system once framework is complete**

---

**KEY LEARNING**: Framework toolbar is a UI-only system. It needs to be enhanced with actual business logic (serialization, file I/O, state management).
