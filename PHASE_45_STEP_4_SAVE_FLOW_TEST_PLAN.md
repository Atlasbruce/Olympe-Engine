# PHASE 45 - STEP 4: TEST BehaviorTreeGraphDocument Save Flow

## 🎯 Objective
Verify that BehaviorTreeGraphDocument::Save() works correctly with single-file persistence and dirty flag clearing.

---

## 📋 TEST PLAN

### **Test 1: Single-File Save**

**Setup:**
1. Create new BehaviorTree graph
2. Add some nodes (e.g., 3-5 nodes)
3. Connect them with links
4. Mark graph as dirty manually or via modifications

**Execution:**
```cpp
// Create document
auto btRenderer = std::make_shared<BehaviorTreeRenderer>(panel);
auto document = std::make_unique<BehaviorTreeGraphDocument>(btRenderer.get());

// Get a valid filepath
std::string filepath = "./test_graphs/test_save.json";

// Call Save
bool result = document->Save(filepath);
```

**Expected Results:**
- ✅ `result` returns true
- ✅ File `test_save.json` exists and contains valid JSON
- ✅ JSON has expected structure (nodes, links, metadata)
- ✅ Console logs show "✓ ============ SAVE SUCCESS ============"

**Verification Code:**
```cpp
// Verify file was created
std::ifstream file(filepath);
ASSERT_TRUE(file.is_open()) << "File not created at " << filepath;

// Parse JSON and verify structure
nlohmann::json j;
file >> j;
file.close();

ASSERT_TRUE(j.contains("nodes")) << "JSON missing 'nodes'";
ASSERT_TRUE(j.contains("links")) << "JSON missing 'links'";
ASSERT_TRUE(j.contains("metadata")) << "JSON missing 'metadata'";
ASSERT_TRUE(j["metadata"].contains("lastModified")) << "Missing timestamp";

// Verify data
int nodeCount = j["nodes"].size();
ASSERT_GT(nodeCount, 0) << "Graph has no nodes";
std::cout << "✓ Saved " << nodeCount << " nodes to file\n";
```

---

### **Test 2: Dirty Flag Clearing**

**Setup:**
1. Load or create a graph
2. Mark it as dirty (`graph->MarkDirty()`)
3. Verify `IsDirty()` returns true

**Execution:**
```cpp
// Initial state
ASSERT_TRUE(document->IsDirty()) << "Graph should be dirty before save";

// Save
bool result = document->Save("./test_graphs/test_dirty.json");
ASSERT_TRUE(result) << "Save should succeed";

// Post-save state
ASSERT_FALSE(document->IsDirty()) << "Graph should be clean after save";
```

**Expected Results:**
- ✅ Before save: `IsDirty()` returns true, "*" shown in UI
- ✅ After successful save: `IsDirty()` returns false, "*" disappears
- ✅ Logs show "Calling NodeGraphManager::SaveGraph()" followed by success message

---

### **Test 3: Validation Error Handling**

**Setup:**
1. Create graph with invalid structure (e.g., missing required fields)
2. Attempt to save

**Execution:**
```cpp
// Create invalid graph (implementation depends on graph structure)
// For now, this is a placeholder - actual test depends on NodeGraph validation rules

bool result = document->Save("./test_graphs/test_invalid.json");
```

**Expected Results:**
- ✅ `result` returns false
- ✅ Logs show "✗ VALIDATION ERROR: [error message]"
- ✅ File is NOT created
- ✅ Dirty flag remains true (not cleared on failed save)

---

### **Test 4: Empty Filepath Handling**

**Setup:**
1. Create document with no filepath set

**Execution:**
```cpp
// Try to save to empty path
bool result = document->Save("");
```

**Expected Results:**
- ✅ `result` returns false
- ✅ Logs show error message about empty filepath
- ✅ No file is created

---

### **Test 5: Filepath Persistence**

**Setup:**
1. Save graph to "path1.json"
2. Verify filepath is stored
3. Save again without providing path (optional - if supported)

**Execution:**
```cpp
// First save
document->Save("./graphs/my_tree_v1.json");
std::string path1 = document->GetFilePath();
ASSERT_EQ(path1, "./graphs/my_tree_v1.json") << "Filepath not updated after save";

// Can verify path persists
ASSERT_EQ(document->GetFilePath(), path1) << "Filepath should persist";
```

**Expected Results:**
- ✅ `GetFilePath()` returns the saved filepath
- ✅ Subsequent calls return the same path

---

### **Test 6: ImNodes Sync Verification**

**Setup:**
1. Load a BehaviorTree with ImNodes visual layout
2. Move nodes in ImNodes UI
3. Save

**Execution:**
```cpp
// In a running editor session:
// 1. Open BehaviorTree editor
// 2. Visually move nodes around in ImNodes
// 3. Call Save

// Logs should show:
// "[BehaviorTreeGraphDocument::Save] Syncing node positions from ImNodes..."
// "[BehaviorTreeGraphDocument::Save] ✓ Position sync complete"
```

**Expected Results:**
- ✅ Node positions are synchronized before save
- ✅ Logs confirm sync operation
- ✅ After reloading file, nodes appear in new positions

---

## 🔧 IMPLEMENTATION NOTES

### **Critical Logging Points to Verify**

These logs should appear when Save is called:

```
✓ [BehaviorTreeGraphDocument::Save] ============ SAVE START ============
✓ [BehaviorTreeGraphDocument::Save] Target filepath: '...'
✓ [BehaviorTreeGraphDocument::Save] Active graphId: <id>
✓ [BehaviorTreeGraphDocument::Save] Graph found with <count> nodes
✓ [BehaviorTreeGraphDocument::Save] Validating graph...
✓ [BehaviorTreeGraphDocument::Save] ✓ Validation passed
✓ [BehaviorTreeGraphDocument::Save] Syncing node positions from ImNodes...
✓ [BehaviorTreeGraphDocument::Save] ✓ Position sync complete
✓ [BehaviorTreeGraphDocument::Save] Calling NodeGraphManager::SaveGraph()
✓ [BehaviorTreeGraphDocument::Save] ✓ ============ SAVE SUCCESS ============
✓ [BehaviorTreeGraphDocument::Save] Saved to: '...'
```

If any step shows an error (✗), save failed and dirty flag is NOT cleared.

---

## ✅ SUCCESS CRITERIA

All tests pass:
- [ ] Test 1: Single-file save works, file is valid JSON
- [ ] Test 2: Dirty flag cleared only after successful save
- [ ] Test 3: Invalid graph cannot be saved
- [ ] Test 4: Empty filepath handled gracefully
- [ ] Test 5: Filepath stored and persists
- [ ] Test 6: ImNodes positions synchronized and persisted

---

## 🚀 NEXT STEP (Step 5)

Wire CanvasToolbarRenderer buttons to BehaviorTreeGraphDocument::Save()

This involves:
- Connect OnSaveClicked() to document->Save()
- Connect OnSaveAsClicked() to modal flow
- Verify button states update based on dirty flag
