# Blueprint Editor Architecture Diagram

## Event Flow for ImGui Interactivity (Part A)

```
SDL Events
    ↓
SDL_AppEvent()
    ↓
ImGui_ImplSDL3_ProcessEvent(event) ← Process ImGui events FIRST
    ↓
ImGui::GetIO().WantCaptureMouse? ───┐
ImGui::GetIO().WantCaptureKeyboard? │
    ↓                                │
    YES → Skip game event            │ NO → Propagate to game
          (ImGui consumes it)        └──→ InputsManager::HandleEvent()
                                              ↓
                                         Game Systems
```

## Entity Tracking Flow (Part B)

```
World::CreateEntity()
    ↓
NotifyBlueprintEditorEntityCreated(entityId)
    ↓
WorldBridge::NotifyEditorEntityCreated()
    ├──→ BlueprintEditor::NotifyEntityCreated()
    │       ↓
    │    m_RuntimeEntities.push_back(entityId)
    │       ↓
    └──→ EntityInspectorManager::OnEntityCreated()
            ↓
         Cache entity info

AssetBrowser::Render()
    ↓
BlueprintEditor::GetRuntimeEntities()
    ↓
For each entity:
    EntityInspectorManager::GetEntityInfo()
    ↓
Display in "Runtime Entities" section
```

## Panel Synchronization Flow (Part C)

```
User clicks entity in AssetBrowser
    ↓
BlueprintEditor::SetSelectedEntity(entityId)
    ↓
m_SelectedEntity = entityId (Backend storage)
    ↓
    ├──────────────┬──────────────┬──────────────┐
    ↓              ↓              ↓              ↓
Inspector    NodeGraph      Entities      Properties
  Panel        Panel         Panel          Panel
    ↓              ↓              ↓              ↓
GetSelectedEntity() from backend (reactive)
    ↓              ↓              ↓              ↓
Display entity    Show entity   Highlight     Show editable
components        name/BT       selection     properties

All panels automatically synchronized through shared backend state!
```

## Menu System Structure (Part D)

```
BlueprintEditorGUI::Render()
    ↓
ImGui::BeginMainMenuBar()
    ├──→ File Menu
    │    ├─ New Blueprint (Ctrl+N) → NewBlueprint()
    │    ├─ Open (Ctrl+O) → LoadBlueprint()
    │    ├─ Save (Ctrl+S) → SaveBlueprint()
    │    ├─ Save As (Ctrl+Shift+S) → SaveBlueprintAs()
    │    ├─ Reload Assets → RefreshAssets()
    │    └─ Exit (F2) → SetActive(false)
    │
    ├──→ Edit Menu
    │    ├─ Undo (Ctrl+Z) → [stub]
    │    ├─ Redo (Ctrl+Y) → [stub]
    │    ├─ Add Component → ShowDialog
    │    ├─ Remove Component → RemoveComponent()
    │    └─ Preferences → ShowPreferences
    │
    ├──→ View Menu
    │    ├─ Asset Browser → Toggle m_ShowAssetBrowser
    │    ├─ Asset Info → Toggle m_ShowAssetInfo
    │    ├─ Inspector → Toggle m_ShowInspector
    │    ├─ Node Graph → Toggle m_ShowNodeGraph
    │    ├─ Entities → Toggle m_ShowEntities
    │    ├─ [... 3 more panels ...]
    │    └─ Reset Layout → Set all to true
    │
    └──→ Help Menu
         ├─ Documentation → Open docs
         ├─ Shortcuts → ShowShortcuts
         └─ About → ShowAbout

Conditional Panel Rendering:
    if (m_ShowAssetBrowser) m_AssetBrowser.Render();
    if (m_ShowInspector) m_InspectorPanel.Render();
    ... etc
```

## Class Relationships

```
┌─────────────────────────────────────────────────────────┐
│                    OlympeEngine.cpp                      │
│  - SDL_AppEvent() handles all input                     │
│  - ImGui event processing BEFORE game                   │
│  - Main rendering loop                                  │
└──────────────────────┬──────────────────────────────────┘
                       │
        ┌──────────────┴──────────────┐
        ↓                             ↓
┌──────────────┐            ┌──────────────────┐
│  GameEngine  │            │ BlueprintEditor  │
│   Systems    │            │     (Backend)    │
└──────────────┘            └────────┬─────────┘
                                     │
                    ┌────────────────┼────────────────┐
                    ↓                ↓                ↓
            ┌───────────────┐  ┌─────────────┐  ┌─────────────┐
            │m_RuntimeEntities│  │m_Selected   │  │ Asset Tree │
            │  (vector)      │  │  Entity     │  │            │
            └───────┬────────┘  └──────┬──────┘  └─────┬──────┘
                    │                  │               │
        ┌───────────┴──────────────────┴───────────────┴───────┐
        ↓                                                        ↓
┌──────────────────┐                              ┌──────────────────────┐
│BlueprintEditorGUI│                              │  EntityInspector     │
│   (Frontend)     │                              │    Manager           │
└────────┬─────────┘                              └──────────────────────┘
         │
    ┌────┴────┬────────┬────────┬────────┐
    ↓         ↓        ↓        ↓        ↓
┌────────┐ ┌────┐ ┌────────┐ ┌────┐ ┌──────┐
│ Asset  │ │Ent.│ │Inspect.│ │Node│ │Props │
│Browser │ │Pnl │ │ Panel  │ │Grph│ │Panel │
└────────┘ └────┘ └────────┘ └────┘ └──────┘
    ↑                ↑           ↑       ↑
    └────────────────┴───────────┴───────┘
         All panels read from backend
         (reactive, no circular deps)
```

## Data Flow Summary

1. **Input**: SDL → ImGui → Game (with proper capture checks)
2. **Entity Lifecycle**: World → WorldBridge → BlueprintEditor + EntityInspectorManager
3. **Selection**: Any Panel → BlueprintEditor.SetSelectedEntity() → All Panels Read
4. **Rendering**: Backend data → GUI reads → ImGui displays

## Key Design Principles

- **Single Source of Truth**: Backend holds all state
- **Reactive Updates**: Panels read state each frame, no push notifications
- **Clean Separation**: Backend has no GUI code, GUI has no business logic
- **Event Capture**: ImGui gets priority on input events
- **Extensibility**: Easy to add new panels that auto-sync with selection
