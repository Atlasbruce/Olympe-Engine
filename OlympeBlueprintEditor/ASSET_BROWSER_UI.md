# Asset Browser UI Layout

This document describes the visual layout of the Asset Browser implementation.

## Main Window Layout

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ Olympe Blueprint Editor - SDL3                                        [_][□][X] │
├─────────────────────────────────────────────────────────────────────────────────┤
│ File   Edit   View   Help                                                       │
├───────────────────────────┬─────────────────────────────────────────────────────┤
│                           │                                                     │
│  ┌─ Asset Browser ─────┐ │  ┌─ Entity Properties ──────────────────────────┐  │
│  │                      │ │  │                                              │  │
│  │ [Search...] [All ▼] │ │  │  Blueprint: SimpleCharacter *                │  │
│  │ [Refresh]           │ │  │  ─────────────────────────────────           │  │
│  │ ──────────────────  │ │  │  Description:                                │  │
│  │                      │ │  │  A basic character entity...                 │  │
│  │ 📁 AI               │ │  │  ─────────────────────────────────           │  │
│  │   ├─ guard_combat...│ │  │  Components (3)                              │  │
│  │   ├─ guard_patrol...│ │  │  ▸ Position                                  │  │
│  │   ├─ idle.json [BT] │ │  │  ▸ BoundingBox                               │  │
│  │   └─ investigate... │ │  │  ▸ VisualSprite                              │  │
│  │                      │ │  │  ─────────────────────────────────           │  │
│  │ 📄 example_entity... │ │  │  [Add Component]                             │  │
│  │ 📄 example_entity... │ │  │                                              │  │
│  │                      │ │  └──────────────────────────────────────────────┘  │
│  │                      │ │                                                     │
│  └──────────────────────┘ │  ┌─ Asset Info ─────────────────────────────────┐  │
│                           │  │                                              │  │
│                           │  │  Entity Blueprint                            │  │
│                           │  │  ─────────────────────────────────           │  │
│                           │  │  Name: SimpleCharacter                       │  │
│                           │  │  Description: A basic character entity...    │  │
│                           │  │                                              │  │
│                           │  │  Components: 3                               │  │
│                           │  │  ▼ Component List                            │  │
│                           │  │    • Position                                │  │
│                           │  │    • BoundingBox                             │  │
│                           │  │    • VisualSprite                            │  │
│                           │  │                                              │  │
│                           │  │  ─────────────────────────────────           │  │
│                           │  │  File: ../Blueprints/example_entity_simple...│  │
│                           │  │                                              │  │
│                           │  └──────────────────────────────────────────────┘  │
│                           │                                                     │
├───────────────────────────┴─────────────────────────────────────────────────────┤
│ Ready | 6 assets found                                                          │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Panel Descriptions

### Asset Browser Panel (Left)

**Location:** Left side of window
**Features:**
- Search text box for filtering by name
- Type filter dropdown (All, EntityBlueprint, BehaviorTree, etc.)
- Refresh button to rescan directories
- Tree view with expandable folders
- Asset icons (📁 for folders, 📄 for files)
- Asset type tags in brackets [BT], [Entity], etc.

**Interactions:**
- Single-click: Select asset (updates Asset Info panel)
- Double-click: Load asset into editor
- Expand/collapse folders with arrow icons

### Asset Info Panel (Right/Bottom)

**Location:** Right side or bottom of window
**Features:**
- Asset type header (color-coded)
  - Green for Entity Blueprints
  - Blue for Behavior Trees
  - Red for errors
- Asset name and description
- Type-specific information:
  - **Entity Blueprints:** Component count and list
  - **Behavior Trees:** Node count and structure
- Collapsible sections for detailed information
- File path display

### Example States

#### State 1: Entity Blueprint Selected
```
┌─ Asset Info ──────────────────────┐
│ Entity Blueprint                  │
│ ───────────────────────────────── │
│ Name: SimpleCharacter             │
│ Description: A basic character... │
│                                   │
│ Components: 3                     │
│ ▼ Component List                  │
│   • Position                      │
│   • BoundingBox                   │
│   • VisualSprite                  │
│ ───────────────────────────────── │
│ File: .../example_entity_simple...│
└───────────────────────────────────┘
```

#### State 2: Behavior Tree Selected
```
┌─ Asset Info ──────────────────────┐
│ Behavior Tree                     │
│ ───────────────────────────────── │
│ Name: GuardPatrolTree             │
│ Behavior Tree AI Definition...   │
│                                   │
│ Nodes: 5                          │
│ ▼ Node List                       │
│   • Root Sequence (Sequence)      │
│   • Pick Next Patrol Point (Act..)│
│   • Set Move Goal... (Action)     │
│   • Move to Patrol Point (Action) │
│   • Wait at Point (Action)        │
│ ───────────────────────────────── │
│ File: .../AI/guard_patrol.json    │
└───────────────────────────────────┘
```

#### State 3: Error State (Malformed JSON)
```
┌─ Asset Info ──────────────────────┐
│ Error Loading Asset               │
│ ───────────────────────────────── │
│ File: .../broken_asset.json       │
│                                   │
│ Error:                            │
│ JSON Parse Error: Expected ']' at│
│ line 15, column 3                 │
│                                   │
│ The JSON file may be corrupted or│
│ malformed. Please check the file  │
│ syntax.                           │
└───────────────────────────────────┘
```

#### State 4: No Asset Selected
```
┌─ Asset Info ──────────────────────┐
│ No asset selected                 │
│                                   │
│ Select an asset from the Asset    │
│ Browser to view details.          │
│                                   │
└───────────────────────────────────┘
```

#### State 5: Search Active
```
┌─ Asset Browser ────────────────┐
│ [patrol   ] [All       ▼]      │
│ [Refresh]                      │
│ ────────────────────────────── │
│ Showing 1 of 6 assets          │
│                                │
│ 📁 AI                          │
│   └─ guard_patrol.json [BT]    │
│                                │
└────────────────────────────────┘
```

#### State 6: Type Filter Active
```
┌─ Asset Browser ────────────────┐
│ [        ] [BehaviorTree ▼]    │
│ [Refresh]                      │
│ ────────────────────────────── │
│ Showing 4 of 6 assets          │
│                                │
│ 📁 AI                          │
│   ├─ guard_combat.json [BT]    │
│   ├─ guard_patrol.json [BT]    │
│   ├─ idle.json [BT]            │
│   └─ investigate.json [BT]     │
│                                │
└────────────────────────────────┘
```

## Color Scheme

### Asset Browser
- Background: Dark gray (#2D2D30)
- Text: Light gray (#CCCCCC)
- Selected item: Blue highlight (#007ACC)
- Hover: Lighter gray (#3E3E42)

### Asset Info Panel
- Header colors:
  - Entity Blueprint: Green (#4EC9B0)
  - Behavior Tree: Blue (#569CD6)
  - Generic: Yellow (#DCDCAA)
  - Error: Red (#F48771)
- Background: Dark gray (#2D2D30)
- Text: Light gray (#CCCCCC)

## Keyboard Shortcuts

- `Ctrl+O` - Open selected asset
- `F5` - Refresh asset browser
- `Ctrl+F` - Focus search box
- `Escape` - Clear search
- `↑/↓` - Navigate asset list
- `Enter` - Load selected asset

## User Workflow

### Opening an Asset
1. User launches the editor
2. Asset Browser automatically scans `Blueprints/` directory
3. User browses folders or uses search
4. User clicks on an asset to select it
5. Asset Info panel updates with metadata
6. User double-clicks to load asset into editor

### Searching for Assets
1. User types in search box (e.g., "patrol")
2. Tree filters to show only matching assets
3. Folders with no matching children are hidden
4. User clicks or double-clicks filtered result

### Filtering by Type
1. User clicks type filter dropdown
2. Selects asset type (e.g., "BehaviorTree")
3. Tree shows only assets of that type
4. User can combine with text search

## Implementation Notes

### ImGui Widgets Used
- `ImGui::Begin()` / `ImGui::End()` - Window panels
- `ImGui::InputText()` - Search box
- `ImGui::BeginCombo()` / `ImGui::EndCombo()` - Type filter dropdown
- `ImGui::Button()` - Refresh button
- `ImGui::TreeNodeEx()` / `ImGui::TreePop()` - Tree structure
- `ImGui::Selectable()` - Selection items
- `ImGui::Text()` / `ImGui::TextColored()` - Text display
- `ImGui::Separator()` - Visual separators
- `ImGui::CollapsingHeader()` - Expandable sections

### Performance Considerations
- Directory scanning done once at startup
- Filtering performed in-memory (no disk I/O)
- Tree nodes rendered on-demand (ImGui handles this)
- Large directories (1000+ files) not yet performance tested

### Future UI Enhancements
- Asset thumbnails/icons
- Drag-and-drop support
- Context menu (right-click)
- Breadcrumb navigation
- Multi-column view option
- Recent files quick access
- Favorites/bookmarks panel
