# Asset Explorer Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         OLYMPE BLUEPRINT EDITOR                              │
│                         Asset Explorer Multi-Folder                          │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              FRONTEND (GUI)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────┐         ┌───────────────────────────┐          │
│  │   AssetBrowser Panel   │         │  AssetInfoPanel Panel     │          │
│  ├────────────────────────┤         ├───────────────────────────┤          │
│  │ • Tree View Rendering  │         │ • Metadata Display        │          │
│  │ • User Interaction     │         │ • Type-specific Rendering │          │
│  │ • Selection Tracking   │         │ • Error Display           │          │
│  │ • Filter UI (Search)   │         │ • Component/Node Lists    │          │
│  │ • Type Filter Dropdown │         │ • Color-coded Types       │          │
│  │ • Refresh Button       │         │                           │          │
│  └──────────┬─────────────┘         └─────────────┬─────────────┘          │
│             │                                     │                         │
│             │ Queries backend for data            │ Queries backend         │
│             │ No direct file access               │ No JSON parsing         │
│             └─────────────────┬───────────────────┘                         │
│                               │                                             │
└───────────────────────────────┼─────────────────────────────────────────────┘
                                │
                                │ Backend API Calls
                                │
┌───────────────────────────────┼─────────────────────────────────────────────┐
│                               │                                             │
│                               ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    BlueprintEditor (Singleton)                      │  │
│  │                         BACKEND API                                 │  │
│  ├─────────────────────────────────────────────────────────────────────┤  │
│  │                                                                     │  │
│  │  Asset Management API:                                              │  │
│  │  • SetAssetRootPath(path)      → Set root, trigger rescan          │  │
│  │  • RefreshAssets()             → Recursive directory scan           │  │
│  │  • GetAssetTree()              → Get tree structure                 │  │
│  │  • GetAllAssets()              → Get flat list of all assets        │  │
│  │  • GetAssetsByType(type)       → Filter assets by type              │  │
│  │  • SearchAssets(query)         → Case-insensitive search            │  │
│  │  • GetAssetMetadata(path)      → Get detailed metadata              │  │
│  │  • DetectAssetType(path)       → Identify asset type                │  │
│  │  • GetLastError()              → Error state management             │  │
│  │                                                                     │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                 │                                          │
│                                 │ Internal Operations                      │
│                                 ▼                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    Internal Backend Logic                           │  │
│  ├─────────────────────────────────────────────────────────────────────┤  │
│  │                                                                     │  │
│  │  • ScanDirectory(path)         → Recursive filesystem scan          │  │
│  │  • ParseAssetMetadata(path)    → JSON parsing & extraction          │  │
│  │  • ParseEntityBlueprint(json)  → Entity-specific parsing            │  │
│  │  • ParseBehaviorTree(json)     → BehaviorTree-specific parsing      │  │
│  │  • CollectAllAssets(node)      → Tree traversal & collection        │  │
│  │                                                                     │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                 │                                          │
└─────────────────────────────────┼──────────────────────────────────────────┘
                                  │
                                  │ File System Access
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          FILE SYSTEM LAYER                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  • std::filesystem (C++17)                                                   │
│  • JsonHelper utilities                                                      │
│  • nlohmann::json parser                                                     │
│  • Cross-platform file operations                                            │
│                                                                              │
│  Blueprints/                                                                 │
│  ├── example_entity_simple.json        [EntityBlueprint]                    │
│  ├── example_entity_complete.json      [EntityBlueprint]                    │
│  └── AI/                                                                     │
│      ├── guard_patrol.json             [BehaviorTree]                       │
│      ├── guard_combat.json             [BehaviorTree]                       │
│      ├── investigate.json              [BehaviorTree]                       │
│      └── idle.json                     [BehaviorTree]                       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘


DATA FLOW:

1. Initialization
   ═══════════════
   
   GUI.Initialize()
        │
        ├─> AssetBrowser.Initialize("Blueprints")
        │       │
        │       └─> Backend.SetAssetRootPath("Blueprints")
        │               │
        │               └─> Backend.RefreshAssets()
        │                       │
        │                       └─> ScanDirectory() [Recursive]
        │                               │
        │                               └─> DetectAssetType() [Per File]
        │
        └─> AssetInfoPanel ready for queries


2. User Selection
   ═══════════════
   
   User clicks asset in AssetBrowser
        │
        ├─> AssetBrowser stores selected path
        │
        └─> GUI detects selection change
                │
                └─> AssetInfoPanel.LoadAsset(path)
                        │
                        └─> Backend.GetAssetMetadata(path)
                                │
                                └─> ParseAssetMetadata()
                                        │
                                        ├─> ParseEntityBlueprint() [if Entity]
                                        └─> ParseBehaviorTree()    [if BT]


3. Search & Filter
   ════════════════
   
   User types in search box
        │
        └─> AssetBrowser updates filter state
                │
                └─> RenderTreeNode() applies filter
                        │
                        ├─> Backend.GetAssetTree() [for structure]
                        │
                        └─> PassesFilter() [client-side]
                                │
                                └─> Display matching nodes only


4. Refresh
   ════════
   
   User clicks Refresh button
        │
        └─> AssetBrowser.Refresh()
                │
                └─> Backend.RefreshAssets()
                        │
                        └─> ScanDirectory() [Full rescan]


SEPARATION OF CONCERNS:

Backend (blueprinteditor.h/.cpp)
────────────────────────────────
✓ Business Logic
✓ File System Access
✓ JSON Parsing
✓ Data Structures (AssetNode, AssetMetadata)
✓ Asset Scanning
✓ Type Detection
✓ Error Management
✗ NO UI Code
✗ NO Rendering

Frontend (AssetBrowser, AssetInfoPanel)
───────────────────────────────────────
✓ ImGui Rendering
✓ User Interaction
✓ UI State Management
✓ Visual Filtering
✓ Selection Tracking
✗ NO File Access
✗ NO JSON Parsing
✗ NO Business Logic


SUPPORTED ASSET TYPES:

EntityBlueprint
───────────────
Detection: "components" array or "type": "EntityBlueprint"
Display:   Green color
Info:      Component count, component types list

BehaviorTree
────────────
Detection: "rootNodeId" + "nodes" array
Display:   Blue color
Info:      Node count, node types/names list

Generic
───────
Detection: Valid JSON, no specific markers
Display:   Yellow color
Info:      Name, description

Unknown
───────
Detection: Invalid/unparseable JSON
Display:   Red error panel
Info:      Error message


ERROR HANDLING:

File System Errors
──────────────────
• Directory not found     → Backend error state
• Permission denied       → Caught, logged, error state
• Invalid path            → Validated, error state

JSON Parse Errors
─────────────────
• Malformed JSON          → AssetMetadata.isValid = false
• Missing fields          → Defaults applied via JsonHelper
• Type mismatches         → Safe getters with fallbacks

Display Errors
──────────────
• Backend error           → Shown in AssetBrowser
• Asset error             → Shown in AssetInfoPanel error panel
• All errors logged       → Console output for debugging


ACCEPTANCE CRITERIA:

✅ Navigate toute l'arborescence Blueprints/ (multi-dossier)
✅ Assets affichés avec méta-info complète
✅ Recherche/filtre opérationnels
✅ Cycle de vie UI/éditeur synchronisés
✅ Code modulaire, scalable
✅ Backend responsable de l'exploration
✅ Frontend GUI affichant via backend
✅ Robustesse multi-plateforme
✅ API documentée
✅ Tests fournis
✅ README mis à jour
```
