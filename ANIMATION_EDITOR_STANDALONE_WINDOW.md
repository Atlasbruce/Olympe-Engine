# Animation Editor Standalone Window Implementation

## Overview
This document describes the implementation of the Animation Editor as a standalone SDL3 window, following the same pattern as the Behavior Tree Debugger.

## Changes Made

### 1. AnimationEditorWindow.h
**Added forward declarations:**
- `struct SDL_Window;`
- `struct SDL_Renderer;`
- `struct ImGuiContext;`

**Added public methods:**
- `void Update(float deltaTime)` - Updates and renders the separate window
- `void ProcessEvent(SDL_Event* event)` - Processes SDL events for the separate window

**Added private members:**
```cpp
// Standalone Window Management
SDL_Window* m_separateWindow = nullptr;
SDL_Renderer* m_separateRenderer = nullptr;
ImGuiContext* m_separateImGuiContext = nullptr;
```

**Added private methods:**
- `void CreateSeparateWindow()` - Creates the separate SDL window with ImGui context
- `void DestroySeparateWindow()` - Properly cleans up the separate window
- `void RenderSeparateWindow()` - Renders the UI in the separate window

### 2. AnimationEditorWindow.cpp

#### CreateSeparateWindow()
- Creates a 1280x720 resizable SDL3 window
- Creates a separate SDL renderer
- Creates a separate ImGui context (isolated from main window)
- Initializes ImGui backends for the separate window
- Saves and restores the main ImGui context

#### DestroySeparateWindow()
- Properly shuts down ImGui backends
- Destroys the ImGui context
- Destroys the SDL renderer
- Destroys the SDL window
- Handles context switching correctly

#### RenderSeparateWindow()
- Switches to the separate ImGui context
- Clears the window (dark gray background)
- Starts ImGui frame
- Calls the existing `Render()` method
- Presents the frame
- Restores the main ImGui context

#### ProcessEvent()
- Filters events for the separate window
- Handles window close button (calls Toggle() to close)
- Routes other events to ImGui in the separate context
- Properly switches and restores ImGui context

#### Update()
- Called every frame from OlympeEngine.cpp
- Updates the preview animation
- Renders the separate window

#### Toggle()
- Creates the separate window if it doesn't exist
- Shows or hides the window based on state
- Handles unsaved changes prompt on close

#### Destructor
- Calls `DestroySeparateWindow()` to ensure cleanup

### 3. OlympeEngine.cpp

#### Event Routing (SDL_AppEvent)
**Before:**
```cpp
// Route events to BT Debugger separate window
if (g_btDebugWindow != nullptr)
{
    g_btDebugWindow->ProcessEvent(event);
}
```

**After:**
```cpp
// Route events to Animation Editor separate window
if (animationEditorWindow != nullptr)
{
    animationEditorWindow->ProcessEvent(event);
}

// Route events to BT Debugger separate window
if (g_btDebugWindow != nullptr)
{
    g_btDebugWindow->ProcessEvent(event);
}
```

#### Rendering (SDL_AppIterate)
**Before:**
```cpp
// Update Animation Editor preview (before rendering)
if (animationEditorWindow && animationEditorWindow->IsOpen())
{
    animationEditorWindow->UpdatePreview(GameEngine::fDt);
}

// ... later ...

// Render Animation Editor Window
if (animationEditorWindow && animationEditorWindow->IsOpen())
{
    animationEditorWindow->Render();
}
```

**After:**
```cpp
// Update Animation Editor (separate window)
if (animationEditorWindow)
{
    animationEditorWindow->Update(GameEngine::fDt);
}
```

## Architecture

### Window Lifecycle
1. **Toggle F9** → `Toggle()` → `CreateSeparateWindow()` → Window created and shown
2. **Each Frame** → `Update()` → `UpdatePreview()` + `RenderSeparateWindow()`
3. **Close X** → `ProcessEvent()` → `Toggle()` → Window hidden
4. **Toggle F9 again** → `Toggle()` → Window hidden (not destroyed)
5. **App Quit** → Destructor → `DestroySeparateWindow()` → Resources freed

### ImGui Context Management
- **Main Context**: Used by main window (Blueprint Editor, panels, etc.)
- **Separate Context**: Used exclusively by Animation Editor
- **Context Switching**: Always save and restore main context around operations
- **Benefits**: No interference between windows, independent input handling

### Event Flow
```
SDL Events
    ↓
Animation Editor ProcessEvent (if window open)
    ↓
BT Debugger ProcessEvent (if window open)
    ↓
Main Window ImGui ProcessEvent
    ↓
Game Logic
```

## Testing Checklist

### Basic Functionality
- [ ] F9 opens Animation Editor in separate window (not overlay)
- [ ] F9 again closes the window
- [ ] Close button (X) on window closes it properly

### Window Management
- [ ] Window can be moved independently (e.g., to second monitor)
- [ ] Window can be resized without affecting main game window
- [ ] Window title shows "Animation Editor - Olympe Engine"
- [ ] Window has dark gray background (45, 45, 48)

### Integration Testing
- [ ] F2 (Blueprint Editor) still works in main window
- [ ] F10 (BT Debugger) still works in separate window
- [ ] Both Animation Editor and BT Debugger can be open simultaneously
- [ ] No interference between windows

### Input Testing
- [ ] Mouse input works correctly in Animation Editor window
- [ ] Keyboard input works correctly in Animation Editor window
- [ ] Input in Animation Editor doesn't affect main window
- [ ] Input in main window doesn't affect Animation Editor

### Stability Testing
- [ ] No crashes when opening/closing window multiple times
- [ ] No crashes when closing window via X button
- [ ] No crashes when closing app with window open
- [ ] No memory leaks (check with Valgrind/sanitizers if available)

### Feature Testing
- [ ] All Animation Editor features work in separate window:
  - [ ] Bank list panel
  - [ ] Spritesheet editor panel
  - [ ] Sequence editor panel
  - [ ] Preview panel with playback controls
  - [ ] Properties panel
  - [ ] Menu bar (File, Edit, View)
  - [ ] All dialogs and file operations

## Comparison with BehaviorTreeDebugWindow

| Feature | BehaviorTreeDebugWindow | AnimationEditorWindow |
|---------|-------------------------|----------------------|
| **Window Creation** | SDL_CreateWindowAndRenderer | ✅ Same |
| **Window Size** | 1200x720 | 1280x720 (slightly wider) |
| **Window Flags** | SDL_WINDOW_RESIZABLE | ✅ Same |
| **ImGui Context** | Separate context | ✅ Same |
| **Context Switching** | Save/restore main context | ✅ Same |
| **Event Routing** | ProcessEvent() method | ✅ Same |
| **Window Close** | Toggle() on close button | ✅ Same |
| **Cleanup** | DestroySeparateWindow() | ✅ Same |
| **Rendering** | RenderInSeparateWindow() | RenderSeparateWindow() (same pattern) |

## Key Design Decisions

1. **Window is created on first F9, not in constructor**
   - Matches BT Debugger pattern
   - Avoids creating window when not needed
   - Window persists after first creation (hidden when closed)

2. **Window is hidden, not destroyed on close**
   - Faster to reopen (just show, not recreate)
   - Preserves window position/size
   - Only destroyed on app quit

3. **Event routing order: Animation Editor → BT Debugger → Main Window**
   - Each separate window gets events first
   - Prevents event leaks between windows
   - Main window processes remaining events

4. **ImGui context switching is explicit and consistent**
   - Always save main context before switching
   - Always restore main context after operations
   - Prevents context corruption and crashes

5. **Update() method combines UpdatePreview() and RenderSeparateWindow()**
   - Single entry point from OlympeEngine
   - Cleaner API (one call instead of two)
   - Matches the pattern of other separate windows

## Benefits

1. **Independent Window**: Can be moved to second monitor
2. **No Viewport Interference**: Doesn't share space with game view
3. **Isolated ImGui Context**: No UI conflicts with main window
4. **Better Workflow**: Can see game and editor simultaneously
5. **Consistent Pattern**: Same architecture as BT Debugger

## Technical Notes

- Uses SDL3 API (not SDL2)
- Background color: RGB(45, 45, 48) - dark gray
- Window title: "Animation Editor - Olympe Engine"
- Default size: 1280x720 pixels
- Window is resizable by user
- ImGui style: Dark theme (ImGui::StyleColorsDark())
