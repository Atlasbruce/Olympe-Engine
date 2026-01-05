# 📁 Asset Browser

The Olympe Asset Browser provides comprehensive asset management with search, filtering, and preview capabilities.

---

## 🎯 Overview

The Asset Browser is an integrated tool for:
- Browsing game assets across multiple directories
- Searching assets by name
- Filtering by asset type
- Viewing asset metadata and details
- Quick asset insertion into blueprints

---

## 🚀 Quick Start

### Opening the Asset Browser

The Asset Browser is integrated into the Blueprint Editor:

1. **Open Blueprint Editor** (Press F2)
2. **Asset Browser panel** appears on the left side
3. Start browsing or searching immediately

### Finding an Asset

1. **Browse folders** - Click to expand directories
2. **Search by name** - Type in search box at top
3. **Filter by type** - Use dropdown to show specific types
4. **Select asset** - Click to view details

---

## 📦 Features

### Multi-Folder Support

The browser scans multiple directories:

```
Project Root
├── Blueprints/
│   ├── Enemies/
│   ├── Items/
│   └── NPCs/
├── Blueprints/AI/
│   ├── idle.json
│   ├── patrol.json
│   └── combat.json
└── Resources/
    ├── Sprites/
    └── Audio/
```

All folders are scanned recursively, showing complete asset hierarchy.

### Asset Types

The browser recognizes and categorizes:

#### EntityBlueprint
- Extension: `.json`
- Schema: `"type": "EntityBlueprint"`
- Icon: 🎮 Blueprint icon
- Preview: Component list

#### BehaviorTree
- Extension: `.json`
- Schema: `"type": "BehaviorTree"`
- Icon: 🌲 Tree icon
- Preview: Node structure

#### Texture/Sprite
- Extensions: `.png`, `.jpg`, `.bmp`
- Icon: 🖼️ Image icon
- Preview: Thumbnail (planned)

#### Audio
- Extensions: `.wav`, `.ogg`, `.mp3`
- Icon: 🔊 Audio icon
- Preview: Audio info (duration, format)

#### Generic Data
- Extension: `.json`
- Schema: Generic or unknown
- Icon: 📄 Document icon
- Preview: Raw JSON structure

### Search & Filter

#### Text Search
```
Search box: "player"
Results:
  ✅ player.json
  ✅ player_enemy.json
  ✅ npc_player_follower.json
  ❌ guard.json
```

Search is case-insensitive and searches asset names.

#### Type Filtering
```
Filter: EntityBlueprint
Results:
  ✅ player.json (EntityBlueprint)
  ✅ enemy.json (EntityBlueprint)
  ❌ patrol.json (BehaviorTree)
  ❌ sprite.png (Texture)
```

Combine search and filter for precise results:
```
Search: "enemy"
Filter: EntityBlueprint
Results: Only enemy entity blueprints
```

### Asset Information Panel

When an asset is selected, the info panel shows:

#### EntityBlueprint
- **Name**: Entity name from JSON
- **Description**: Entity description
- **Component Count**: Number of components
- **Component List**: Names of all components
- **File Size**: Size in KB
- **Last Modified**: Modification timestamp

#### BehaviorTree
- **Name**: Tree name
- **Tree ID**: Unique identifier
- **Node Count**: Total nodes in tree
- **Root Node**: Root node ID
- **Node Types**: Breakdown (Actions, Conditions, etc.)
- **File Size**: Size in KB

#### Texture
- **Dimensions**: Width x Height
- **Format**: PNG, JPG, etc.
- **File Size**: Size in KB
- **Preview**: Thumbnail (planned)

#### Audio
- **Duration**: Length in seconds (planned)
- **Format**: WAV, OGG, etc.
- **Sample Rate**: Audio quality (planned)
- **File Size**: Size in KB

---

## 🎨 User Interface

### Layout

```
┌─────────────────────────────────────┐
│  Asset Browser                       │
├─────────────────────────────────────┤
│  [Search: ____________] [Filter: ▼] │
├─────────────────────────────────────┤
│  📁 Blueprints                       │
│    ├─ 📁 Enemies                    │
│    │    ├─ 🎮 enemy_guard.json      │
│    │    └─ 🎮 enemy_flyer.json      │
│    ├─ 📁 Items                      │
│    └─ 📁 NPCs                       │
│  📁 AI                               │
│    ├─ 🌲 idle.json                  │
│    ├─ 🌲 patrol.json                │
│    └─ 🌲 combat.json                │
│  📁 Resources                        │
│    ├─ 🖼️ player.png                 │
│    └─ 🔊 footstep.wav               │
├─────────────────────────────────────┤
│  Asset Info                          │
│  Name: enemy_guard                   │
│  Type: EntityBlueprint              │
│  Components: 6                       │
│  - Position                          │
│  - Health                            │
│  - AIBehavior                        │
│  Size: 2.4 KB                       │
└─────────────────────────────────────┘
```

### Controls

- **Click folder** - Expand/collapse
- **Click asset** - Select and show info
- **Double-click asset** - Open in editor or insert
- **Right-click asset** - Context menu (planned)
- **Drag asset** - Drag to property field (planned)

---

## 🏗️ Architecture

### Backend (AssetBrowser Class)

Singleton managing asset data and operations:

```cpp
class AssetBrowser {
public:
    // Scanning
    void ScanDirectory(const std::string& path, bool recursive);
    void RefreshAssets();
    
    // Querying
    std::vector<AssetInfo> GetAllAssets();
    std::vector<AssetInfo> GetAssetsByType(AssetType type);
    std::vector<AssetInfo> SearchAssets(const std::string& query);
    
    // Asset info
    AssetInfo GetAssetInfo(const std::string& path);
    AssetMetadata GetMetadata(const std::string& path);
};
```

### Frontend (AssetBrowserGUI Class)

ImGui-based UI:

```cpp
class AssetBrowserGUI {
private:
    AssetBrowser* backend;
    std::string searchQuery;
    AssetType filterType;
    
public:
    void Render();
    void RenderSearchBar();
    void RenderAssetTree();
    void RenderInfoPanel();
};
```

### Data Structures

```cpp
enum class AssetType {
    Unknown,
    EntityBlueprint,
    BehaviorTree,
    Texture,
    Audio,
    Data
};

struct AssetInfo {
    std::string path;
    std::string name;
    AssetType type;
    size_t fileSize;
    time_t lastModified;
};

struct AssetMetadata {
    // EntityBlueprint metadata
    std::string entityName;
    std::string description;
    std::vector<std::string> components;
    
    // BehaviorTree metadata
    int treeID;
    int nodeCount;
    std::string rootNodeType;
    
    // Texture metadata
    int width;
    int height;
    std::string format;
};
```

### Design Principles

1. **Separation of Concerns**
   - Backend: File scanning, metadata extraction
   - Frontend: UI rendering, user interaction
   - No direct file I/O from frontend

2. **Caching**
   - Asset list cached after scan
   - Metadata cached per asset
   - Only re-scan on explicit refresh

3. **Performance**
   - Background scanning (planned)
   - Lazy metadata loading
   - Virtual scrolling for large lists (planned)

---

## 🔧 Advanced Features

### Recursive Scanning

The browser scans all subdirectories:

```
Blueprints/
├── player.json           ✅ Found
├── Enemies/
│   ├── guard.json        ✅ Found
│   └── Boss/
│       └── dragon.json   ✅ Found (2 levels deep)
└── Items/
    └── potion.json       ✅ Found
```

### Metadata Extraction

For each asset type, specific metadata is extracted:

#### EntityBlueprint
```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Player",              ← Extracted
    "description": "Main character", ← Extracted
    "components": [                ← Count extracted
        {"type": "Position", ...}, ← Component names extracted
        {"type": "Health", ...}
    ]
}
```

#### BehaviorTree
```json
{
    "tree_id": 2,           ← Extracted
    "name": "patrol",       ← Extracted
    "root_node_id": 1,      ← Extracted
    "nodes": [...]          ← Count extracted
}
```

### Error Handling

Robust error handling for:

- ❌ **Invalid JSON** - Shows "Invalid" in info panel
- ❌ **Missing files** - Removed from list on refresh
- ❌ **Corrupted files** - Error icon, details in info
- ❌ **Access denied** - Skipped with warning

### File Watching (Planned)

Automatic refresh when files change:

```cpp
// File watcher detects change
OnFileChanged("Blueprints/player.json");

// Auto-refresh asset list
browser->RefreshAsset("Blueprints/player.json");

// Update UI if asset is selected
if (selectedAsset == changedAsset) {
    UpdateInfoPanel();
}
```

---

## 💡 Usage Examples

### Finding a Specific Blueprint

```
1. Open Asset Browser (part of Blueprint Editor)
2. Type "player" in search box
3. Filter: EntityBlueprint
4. Results show only player-related blueprints
5. Click on desired blueprint
6. Info panel shows components and details
```

### Browsing AI Behavior Trees

```
1. Open Asset Browser
2. Navigate to AI/ folder
3. See all behavior trees:
   - idle.json
   - patrol.json
   - combat.json
   - investigate.json
4. Select tree to view node count and structure
```

### Inserting an Asset into a Blueprint

```
1. Editing VisualSprite component in Blueprint Editor
2. Need to set spritePath property
3. Open Asset Browser (or it's already open)
4. Search for sprite name
5. Filter: Texture
6. Double-click sprite or drag to property field
7. Path automatically inserted: "Resources/Sprites/player.png"
```

---

## 🔗 Integration

### With Blueprint Editor

The Asset Browser integrates seamlessly:

```cpp
// In Blueprint Editor, selecting sprite path
void PropertyInspector::RenderSpritePathProperty(Property& prop) {
    if (ImGui::Button("Browse...")) {
        assetBrowser->Show();
        assetBrowser->SetFilter(AssetType::Texture);
        assetBrowser->SetCallback([&prop](const std::string& path) {
            prop.value = path;
        });
    }
}
```

### With ECS System

Assets can be loaded at runtime:

```cpp
// Load entity from browsed asset
std::string blueprintPath = assetBrowser->GetSelectedAsset().path;
EntityBlueprint blueprint = EntityBlueprint::LoadFromFile(blueprintPath);

// Create entity in world
EntityID entity = CreateEntityFromBlueprint(blueprint);
```

---

## 🐛 Troubleshooting

### No Assets Showing

**Problem**: Asset Browser is empty

**Solutions**:
- Verify asset directories exist (Blueprints/, Resources/)
- Check JSON files are valid
- Click "Refresh" button to rescan
- Check console for scan errors

### Search Not Working

**Problem**: Search doesn't find assets

**Solutions**:
- Search is case-insensitive but must match part of filename
- Clear filter to search all types
- Ensure file actually exists in scanned directories

### Wrong Asset Type Detected

**Problem**: Asset shows wrong icon/type

**Solutions**:
- Check JSON schema has correct "type" field
- Verify file extension is correct
- Refresh asset list to re-detect

---

## 📊 Performance

### Scan Performance

Typical scan times:

| Asset Count | Scan Time | Notes |
|-------------|-----------|-------|
| 100 files | < 100ms | Fast |
| 1000 files | < 1s | Acceptable |
| 10000 files | < 10s | Initial scan only |

### Memory Usage

| Asset Count | Memory | Notes |
|-------------|--------|-------|
| 100 | ~50 KB | Asset info only |
| 1000 | ~500 KB | Minimal overhead |
| 10000 | ~5 MB | Still reasonable |

Metadata is loaded on-demand, not cached for all assets.

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 🎨 [Blueprint Editor](BLUEPRINT_EDITOR.md)
- 📋 [Blueprint System](../03-Core-Systems/Blueprint/README.md)
- 🛠️ [Tools Overview](README.md)

---

[← Back to Tools](README.md) | [← Blueprint Editor](BLUEPRINT_EDITOR.md)
