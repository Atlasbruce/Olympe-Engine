# 🛠️ Tools

Olympe Engine includes powerful tools for game development and content creation.

---

## 📖 Tools Documentation

| Tool | Description | Audience |
|------|-------------|----------|
| **[Blueprint Editor](BLUEPRINT_EDITOR.md)** | Visual entity editor with node-based interface | Designers, Developers |
| **[Asset Browser](ASSET_BROWSER.md)** | Multi-folder asset management and exploration | Designers, Developers |

---

## 🎨 Blueprint Editor

The Blueprint Editor is an integrated visual tool for creating and editing entity blueprints without writing code.

### Key Features

- ✅ **Visual Node Editor** - ImGui + ImNodes based interface
- ✅ **Component Editing** - Edit component properties visually
- ✅ **Asset Integration** - Integrated Asset Browser
- ✅ **Runtime Toggle** - Press F2 to open/close
- ✅ **File Management** - Create, load, save blueprints
- ✅ **Backend/Frontend Architecture** - Clean separation for maintainability

### Quick Start

1. **Launch the game engine**
2. **Press F2** to toggle the Blueprint Editor
3. **File → New** to create a new blueprint
4. **Add components** from the component library
5. **Edit properties** in the property inspector
6. **File → Save** to save your blueprint

See: **[Blueprint Editor Documentation](BLUEPRINT_EDITOR.md)**

---

## 📁 Asset Browser

The Asset Browser provides a comprehensive view of all game assets with search, filtering, and preview capabilities.

### Key Features

- ✅ **Multi-Folder Support** - Scan multiple asset directories
- ✅ **Asset Types** - EntityBlueprint, BehaviorTree, and more
- ✅ **Search & Filter** - Quick asset discovery
- ✅ **Metadata Display** - Type-specific information
- ✅ **Tree View** - Hierarchical folder navigation
- ✅ **Backend API** - Clean separation from UI

### Supported Asset Types

- **EntityBlueprint** (.json) - Entity definitions
- **BehaviorTree** (.json) - AI behavior trees
- **Sprite** (.png, .jpg) - Visual assets
- **Audio** (.wav, .ogg) - Sound effects and music
- **Data** (.json) - Generic data files

See: **[Asset Browser Documentation](ASSET_BROWSER.md)**

---

## 🔧 Tool Architecture

### Backend/Frontend Separation

```
┌────────────────────────────────────────────┐
│              Tools Layer                    │
├────────────────────────────────────────────┤
│                                             │
│  ┌─────────────────┐   ┌─────────────────┐│
│  │  Frontend (UI)  │   │  Backend (Data) ││
│  │                 │   │                 ││
│  │  - ImGui        │◄──┤  - Asset Scan   ││
│  │  - User Input   │   │  - Metadata     ││
│  │  - Rendering    │   │  - File I/O     ││
│  │                 │   │  - Validation   ││
│  └─────────────────┘   └─────────────────┘│
│                                             │
└────────────────────────────────────────────┘
            │                    │
            ▼                    ▼
    ┌─────────────┐      ┌─────────────┐
    │  Rendering  │      │  File       │
    │  System     │      │  System     │
    └─────────────┘      └─────────────┘
```

### Design Principles

1. **Separation of Concerns**
   - Frontend handles only UI and user interaction
   - Backend handles data, logic, and file operations
   - No direct file access from frontend

2. **Testability**
   - Backend can be tested independently
   - Mock UI for automated testing
   - Data validation separate from presentation

3. **Scalability**
   - Easy to add new asset types
   - Plugin architecture ready
   - Multiple frontend implementations possible

---

## 🎮 Using Tools in Your Workflow

### Entity Creation Workflow

```
Design Phase          Creation Phase         Runtime Phase
────────────          ──────────────         ─────────────

  Concept     ──►    Blueprint Editor   ──►   Game Engine
   Sketch              (F2 in-game)          CreateEntity()
                           │                      │
                           │                      │
                           ▼                      ▼
                    Save to JSON            Add to World
                  (Blueprints/*.json)      (ECS System)
```

### Asset Management Workflow

```
Import Assets        Organize Assets       Use in Game
─────────────        ───────────────       ───────────

Copy to folder  ──►  Asset Browser    ──►  Load Asset
(Resources/)         Search & Filter       (Runtime)
                     View Metadata
```

---

## 💻 Tool Development

### Adding a New Tool

1. **Create Backend Class**
   ```cpp
   class MyToolBackend {
   public:
       void Initialize();
       void LoadData();
       void ProcessData();
   };
   ```

2. **Create Frontend Class**
   ```cpp
   class MyToolGUI {
   private:
       MyToolBackend* backend;
   public:
       void Render();
       void HandleInput();
   };
   ```

3. **Register with Engine**
   ```cpp
   // In GameEngine::Init()
   myTool = std::make_unique<MyToolGUI>();
   myTool->Initialize();
   ```

4. **Integrate with ImGui**
   ```cpp
   void GameEngine::RenderTools() {
       if (ImGui::BeginMainMenuBar()) {
           if (ImGui::BeginMenu("Tools")) {
               if (ImGui::MenuItem("My Tool")) {
                   myTool->Toggle();
               }
               ImGui::EndMenu();
           }
           ImGui::EndMainMenuBar();
       }
       
       myTool->Render();
   }
   ```

---

## 🔌 Tool Integration

### With Blueprint System

Tools integrate seamlessly with the Blueprint system:

```cpp
// Blueprint Editor can load/save blueprints
EntityBlueprint blueprint = EntityBlueprint::LoadFromFile("path.json");
editor->LoadBlueprint(blueprint);

// Asset Browser displays blueprint metadata
AssetInfo info = browser->GetAssetInfo("player.json");
// info.type == AssetType::EntityBlueprint
```

### With ECS System

Tools can interact with live ECS entities:

```cpp
// Select entity in world
EntityID selected = editor->GetSelectedEntity();

// View/edit components
auto* pos = World::Get().GetComponent<Position_data>(selected);
editor->DisplayPosition(pos);
```

### With Event System

Tools can listen to and emit events:

```cpp
// Listen for asset changes
EventQueue::Subscribe(EventDomain::Editor, EventType::AssetModified, 
    [](const Event& e) {
        // Reload asset
    });

// Emit tool events
Event event(EventDomain::Editor, EventType::BlueprintSaved);
event.data["path"] = "Blueprints/player.json";
EventQueue::Emit(event);
```

---

## 📊 Tool Performance

### Optimization Strategies

1. **Lazy Loading**
   - Load assets only when needed
   - Cache frequently accessed data
   - Unload unused assets

2. **Background Processing**
   - Scan assets on separate thread
   - Generate thumbnails asynchronously
   - Don't block main thread

3. **Incremental Updates**
   - Only refresh changed assets
   - Use file watchers for auto-reload
   - Batch update operations

### Performance Targets

| Operation | Target Time | Notes |
|-----------|-------------|-------|
| Asset Scan | < 1s per 1000 files | Initial scan |
| Asset Search | < 50ms | Interactive |
| Blueprint Load | < 100ms | Small-medium blueprint |
| UI Render | < 16ms | 60 FPS target |

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 📋 [Blueprint System](../03-Core-Systems/Blueprint/README.md)
- 🏗️ [Architecture Overview](../02-Architecture/README.md)

---

## 📖 Detailed Tool Documentation

- **[Blueprint Editor](BLUEPRINT_EDITOR.md)** - Complete editor guide
- **[Asset Browser](ASSET_BROWSER.md)** - Asset management guide

---

[← Back to Documentation Hub](../README.md) | [Next: Development →](../06-Development/README.md)
