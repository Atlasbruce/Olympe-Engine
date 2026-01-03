# Blueprint Editor Architecture

## Overview

The Blueprint Editor is a visual entity blueprint editor integrated into the Olympe Engine. It features a clean separation between backend (business logic) and frontend (user interface), following modern software architecture principles.

## Architecture

### Backend: `BlueprintEditor` (Singleton)

**Location**: `Source/BlueprintEditor/BlueprintEditor.h/cpp`

The backend is a singleton that manages all business logic, state, and data:

#### Responsibilities
- **State Management**: Active/inactive state, unsaved changes tracking
- **Data Management**: Current blueprint, file path, asset root path
- **Business Operations**: 
  - `NewBlueprint()` - Create new blueprint
  - `LoadBlueprint()` - Load from file
  - `SaveBlueprint()` - Save current blueprint
  - `SaveBlueprintAs()` - Save to new location
- **Lifecycle**: Initialize, Update, Shutdown

#### Key Methods
```cpp
// Singleton access
static BlueprintEditor& Instance();
static BlueprintEditor& Get();

// Lifecycle
void Initialize();
void Shutdown();
void Update(float deltaTime);  // Called by GameEngine

// State control
bool IsActive() const;
void ToggleActive();
void SetActive(bool active);

// Blueprint operations
void NewBlueprint(const std::string& name, const std::string& description = "");
bool LoadBlueprint(const std::string& filepath);
bool SaveBlueprint();
bool SaveBlueprintAs(const std::string& filepath);

// Data access
const Blueprint::EntityBlueprint& GetCurrentBlueprint() const;
Blueprint::EntityBlueprint& GetCurrentBlueprintMutable();

// State queries
bool HasBlueprint() const;
bool HasUnsavedChanges() const;
const std::string& GetCurrentFilepath() const;
```

#### Design Principles
- ✅ **No UI Code**: Backend contains zero ImGui or rendering code
- ✅ **Single Responsibility**: Only manages data and business logic
- ✅ **Singleton Pattern**: Global access point for editor state
- ✅ **Clear API**: Well-defined interface for data access and modification

### Frontend: `BlueprintEditorGUI`

**Location**: `Source/BlueprintEditor/BlueprintEditorGUI.h/cpp`

The frontend handles all user interface rendering using ImGui:

#### Responsibilities
- **UI Rendering**: All ImGui panels and windows
- **User Interaction**: Menu bars, dialogs, buttons, node editor
- **Visual Feedback**: Status indicators, tooltips, highlights
- **Delegation**: All data operations delegate to backend

#### Key Components
- **Menu Bar**: File operations, edit operations, view options
- **Entity Panel**: Blueprint properties and component list
- **Node Editor**: Visual node-based component representation
- **Property Panel**: Selected component property display
- **Status Bar**: Current file, modification status
- **Asset Browser**: Navigate and select blueprint files
- **Asset Info Panel**: Display asset metadata

#### Design Principles
- ✅ **No Business Logic**: UI only displays and delegates
- ✅ **Backend Integration**: All data retrieved from `BlueprintEditor::Get()`
- ✅ **Active Check**: Only renders when `BlueprintEditor::IsActive()` is true
- ✅ **Separation of Concerns**: UI state (selected index, dialogs) vs. data state (backend)

## GameEngine Integration

### Initialization

In `OlympeEngine.cpp` (`SDL_AppInit`):

```cpp
// Initialize Blueprint Editor Backend
Olympe::BlueprintEditor::Get().Initialize();

// Create Blueprint Editor GUI
blueprintEditorGUI = new Olympe::BlueprintEditorGUI();
blueprintEditorGUI->Initialize();
```

### F2 Toggle

In `OlympeEngine.cpp` (`SDL_AppEvent`):

```cpp
case SDL_EVENT_KEY_DOWN:
    if (event->key.key == SDLK_F2)
    {
        Olympe::BlueprintEditor::Get().ToggleActive();
    }
```

### Update Loop

In `OlympeEngine.cpp` (`SDL_AppIterate`):

```cpp
// Update backend when active
if (Olympe::BlueprintEditor::Get().IsActive())
{
    Olympe::BlueprintEditor::Get().Update(GameEngine::fDt);
}

// Render UI when active
if (Olympe::BlueprintEditor::Get().IsActive() && blueprintEditorGUI)
{
    blueprintEditorGUI->Render();
}
```

### Shutdown

In `OlympeEngine.cpp` (`SDL_AppQuit`):

```cpp
// Shutdown Blueprint Editor
if (blueprintEditorGUI)
{
    blueprintEditorGUI->Shutdown();
    delete blueprintEditorGUI;
}
Olympe::BlueprintEditor::Get().Shutdown();
```

## Data Flow

```
User Interaction (ImGui)
        ↓
BlueprintEditorGUI::Render()
        ↓
Delegate to Backend API
        ↓
BlueprintEditor::Get().Method()
        ↓
Modify Backend Data
        ↓
Return to GUI
        ↓
GUI Displays Updated Data
```

### Example: Loading a Blueprint

```cpp
// User clicks "Open" in GUI menu
void BlueprintEditorGUI::LoadBlueprint(const std::string& filepath)
{
    // Delegate to backend
    if (BlueprintEditor::Get().LoadBlueprint(filepath))
    {
        // Reset UI state on success
        m_SelectedComponentIndex = -1;
        m_NodePositions.clear();
    }
}
```

The backend handles all file I/O, JSON parsing, and data management:

```cpp
bool BlueprintEditor::LoadBlueprint(const std::string& filepath)
{
    Blueprint::EntityBlueprint loaded = Blueprint::EntityBlueprint::LoadFromFile(filepath);
    
    if (loaded.name.empty())
        return false;
    
    m_CurrentBlueprint = loaded;
    m_CurrentFilepath = filepath;
    m_HasUnsavedChanges = false;
    return true;
}
```

## Benefits of This Architecture

### ✅ Maintainability
- Clear separation allows independent modification of UI and logic
- Easy to add new features to either layer without affecting the other
- Testable: Backend can be unit tested without UI

### ✅ Scalability
- Backend can support multiple frontends (CLI, GUI, web interface)
- Easy to add new UI panels without touching business logic
- Future features can extend backend API cleanly

### ✅ Performance
- Backend state management is lightweight
- UI only updates when active
- Clear lifecycle prevents resource leaks

### ✅ Industrial Quality
- Professional separation of concerns
- Follows established design patterns (Singleton, MVC-like)
- Clean interfaces and minimal coupling

## Future Enhancements

### Planned Features
- **ImGui Integration**: Full ImGui integration in main engine loop
- **File Dialogs**: Native file open/save dialogs
- **Undo/Redo**: Command pattern for blueprint modifications
- **Property Editing**: Direct property value editing in GUI
- **Validation**: Real-time blueprint validation
- **Templates**: Blueprint templates for common entity types

### Extensibility Points
- **Custom Panels**: Add new UI panels by extending `BlueprintEditorGUI`
- **Asset Types**: Support additional asset types through backend
- **Export Formats**: Add export to different formats via backend API
- **Plugins**: Plugin system for custom component types

## Usage Guide

### For Developers

#### Adding a New Backend Feature
1. Add method to `BlueprintEditor` class
2. Implement business logic without UI code
3. Provide clean API for GUI to call
4. Update GUI to expose feature to user

#### Adding a New UI Panel
1. Add panel method to `BlueprintEditorGUI`
2. Use ImGui to render panel
3. Get all data from `BlueprintEditor::Get()`
4. Delegate all modifications to backend methods
5. Call panel method in `Render()`

#### Example: Add Component Count Display
```cpp
// Backend - add query method
int BlueprintEditor::GetComponentCount() const
{
    return (int)m_CurrentBlueprint.components.size();
}

// Frontend - display in status bar
void BlueprintEditorGUI::RenderStatusBar()
{
    auto& backend = BlueprintEditor::Get();
    ImGui::Text("Components: %d", backend.GetComponentCount());
}
```

## Testing

### Backend Testing
The backend can be tested independently:

```cpp
// Unit test example
void TestBlueprintOperations()
{
    auto& editor = BlueprintEditor::Get();
    editor.Initialize();
    
    // Test create
    editor.NewBlueprint("TestEntity", "Test description");
    assert(editor.HasBlueprint());
    assert(editor.GetCurrentBlueprint().name == "TestEntity");
    
    // Test save
    bool saved = editor.SaveBlueprintAs("test.json");
    assert(saved);
    assert(!editor.HasUnsavedChanges());
    
    // Test load
    editor.NewBlueprint("Other", "");
    bool loaded = editor.LoadBlueprint("test.json");
    assert(loaded);
    assert(editor.GetCurrentBlueprint().name == "TestEntity");
    
    editor.Shutdown();
}
```

### Integration Testing
Test frontend-backend integration:
- Verify all GUI operations update backend state
- Verify backend state changes reflect in GUI
- Test F2 toggle behavior
- Test lifecycle (initialize, activate, deactivate, shutdown)

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    Game Engine                          │
│  (OlympeEngine.cpp - SDL_AppIterate)                    │
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │  Press F2 → Toggle BlueprintEditor             │    │
│  │  If Active → Update Backend                    │    │
│  │  If Active → Render GUI                        │    │
│  └────────────────────────────────────────────────┘    │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴─────────────┐
        ▼                          ▼
┌──────────────────┐      ┌──────────────────┐
│   Backend        │      │    Frontend      │
│ BlueprintEditor  │◄─────│BlueprintEditorGUI│
│  (Singleton)     │      │                  │
├──────────────────┤      ├──────────────────┤
│ - State          │      │ - UI Rendering   │
│ - Data           │      │ - User Input     │
│ - Business Logic │      │ - Delegation     │
│ - File I/O       │      │ - Visual Feedback│
└──────────────────┘      └──────────────────┘
        │                          │
        │ Uses                     │ Uses
        ▼                          ▼
┌──────────────────┐      ┌──────────────────┐
│ EntityBlueprint  │      │  ImGui Library   │
│   (Data Model)   │      │  ImNodes Library │
└──────────────────┘      └──────────────────┘
```

## Conclusion

This architecture provides a solid foundation for the Blueprint Editor, ensuring:
- **Clean code** with clear responsibilities
- **Easy maintenance** and future development
- **Professional quality** suitable for production
- **Extensibility** for future features

The separation between backend and frontend is the key to scalable, maintainable, and testable code.
