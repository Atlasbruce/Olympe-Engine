# How to Create a Standalone Window in Olympe Engine

## Overview
This guide explains how to create a new editor or debugger window that runs in its own SDL3 window, separate from the main game window. This pattern is used by the BehaviorTreeDebugWindow and AnimationEditorWindow.

## When to Use This Pattern

✅ **Use standalone window when:**
- The tool needs significant screen space
- Users want to move tool to second monitor
- Tool should not interfere with game viewport
- Tool has complex UI with many panels
- Tool is used for extended periods

❌ **Don't use standalone window when:**
- Simple overlay is sufficient (e.g., FPS counter)
- Tool is used briefly (e.g., quick settings)
- Tool is tightly coupled to main window rendering
- Screen space is limited (mobile platforms)

## Step-by-Step Implementation

### 1. Update Header File (YourWindow.h)

#### Add Forward Declarations
```cpp
// Forward declarations
struct SDL_Window;
struct SDL_Renderer;
struct ImGuiContext;
```

#### Add Private Members
```cpp
private:
    // Standalone Window Management
    SDL_Window* m_separateWindow = nullptr;
    SDL_Renderer* m_separateRenderer = nullptr;
    ImGuiContext* m_separateImGuiContext = nullptr;
    
    void CreateSeparateWindow();
    void DestroySeparateWindow();
    void RenderSeparateWindow();
```

#### Add Public Methods
```cpp
public:
    /**
     * @brief Update and render the window (call every frame)
     * @param deltaTime Time elapsed since last frame in seconds
     */
    void Update(float deltaTime);

    /**
     * @brief Process SDL events for the separate window
     * @param event SDL event to process
     */
    void ProcessEvent(SDL_Event* event);
```

### 2. Update Implementation File (YourWindow.cpp)

#### Add Includes
```cpp
#include "YourWindow.h"
#include "../GameEngine.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
```

#### Implement CreateSeparateWindow()
```cpp
void YourWindow::CreateSeparateWindow()
{
    if (m_separateWindow) 
    {
        SYSTEM_LOG << "[YourWindow] Separate window already exists\n";
        return;
    }
    
    // Save current context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    
    // Create SDL window (adjust size as needed)
    if (!SDL_CreateWindowAndRenderer(
        "Your Window Title - Olympe Engine",
        1280,  // width
        720,   // height
        SDL_WINDOW_RESIZABLE,
        &m_separateWindow,
        &m_separateRenderer))
    {
        SYSTEM_LOG << "[YourWindow] Failed to create window: " << SDL_GetError() << "\n";
        return;
    }
    
    // Create separate ImGui context
    m_separateImGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Initialize ImGui backends
    ImGui_ImplSDL3_InitForSDLRenderer(m_separateWindow, m_separateRenderer);
    ImGui_ImplSDLRenderer3_Init(m_separateRenderer);
    
    // Restore main context
    ImGui::SetCurrentContext(mainContext);
    
    SYSTEM_LOG << "[YourWindow] Standalone window created\n";
}
```

#### Implement DestroySeparateWindow()
```cpp
void YourWindow::DestroySeparateWindow()
{
    if (!m_separateWindow)
        return;
    
    // Save current context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    
    if (m_separateImGuiContext)
    {
        ImGui::SetCurrentContext(m_separateImGuiContext);
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(m_separateImGuiContext);
        m_separateImGuiContext = nullptr;
    }
    
    // Restore main context
    ImGui::SetCurrentContext(mainContext);
    
    if (m_separateRenderer)
    {
        SDL_DestroyRenderer(m_separateRenderer);
        m_separateRenderer = nullptr;
    }
    
    if (m_separateWindow)
    {
        SDL_DestroyWindow(m_separateWindow);
        m_separateWindow = nullptr;
    }
    
    SYSTEM_LOG << "[YourWindow] Separate window destroyed\n";
}
```

#### Implement RenderSeparateWindow()
```cpp
void YourWindow::RenderSeparateWindow()
{
    if (!m_separateWindow || !m_separateRenderer) 
        return;
    
    // Switch to separate ImGui context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    
    // Clear window (customize background color)
    SDL_SetRenderDrawColor(m_separateRenderer, 45, 45, 48, 255);
    SDL_RenderClear(m_separateRenderer);
    
    // ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    // === YOUR UI RENDERING CODE HERE ===
    // Example:
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Your Window##Main", nullptr, flags))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit")) { Toggle(); }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        ImGui::Text("Your content here");
        
        ImGui::End();
    }
    // === END YOUR UI RENDERING CODE ===
    
    // Present
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_separateRenderer);
    SDL_RenderPresent(m_separateRenderer);
    
    // Restore main context
    ImGui::SetCurrentContext(mainContext);
}
```

#### Implement ProcessEvent()
```cpp
void YourWindow::ProcessEvent(SDL_Event* event)
{
    if (!m_separateWindow || !m_isOpen) 
        return;
    
    // Check if event is for our window
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        Uint32 windowID = SDL_GetWindowID(m_separateWindow);
        if (event->window.windowID == windowID)
        {
            Toggle(); // Close window
            return;
        }
    }
    
    // Forward event to ImGui (separate context)
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    ImGui_ImplSDL3_ProcessEvent(event);
    ImGui::SetCurrentContext(mainContext);
}
```

#### Implement Update()
```cpp
void YourWindow::Update(float deltaTime)
{
    if (!m_isOpen) 
        return;
    
    // Update your data/state here
    // Example: UpdateAnimation(deltaTime);
    
    // Render separate window
    RenderSeparateWindow();
}
```

#### Update Toggle()
```cpp
void YourWindow::Toggle()
{
    m_isOpen = !m_isOpen;
    
    if (m_isOpen)
    {
        // Create separate window if it doesn't exist
        if (!m_separateWindow)
        {
            CreateSeparateWindow();
        }
        
        // Show the window
        if (m_separateWindow)
        {
            SDL_ShowWindow(m_separateWindow);
        }
        
        SYSTEM_LOG << "YourWindow: Opened\n";
    }
    else
    {
        // Hide the window
        if (m_separateWindow)
        {
            SDL_HideWindow(m_separateWindow);
        }
        
        SYSTEM_LOG << "YourWindow: Closed\n";
    }
}
```

#### Update Destructor
```cpp
YourWindow::~YourWindow()
{
    DestroySeparateWindow();
    SYSTEM_LOG << "YourWindow: Destroyed\n";
}
```

### 3. Integrate into OlympeEngine.cpp

#### Add Global Pointer (near top of file)
```cpp
static YourWindow* g_yourWindow = nullptr;
```

#### Initialize (in SDL_AppInit)
```cpp
// Create Your Window
g_yourWindow = new YourWindow();
SYSTEM_LOG << "Your Window initialized (toggle with FX)" << endl;
```

#### Add Event Routing (in SDL_AppEvent, BEFORE main window ImGui)
```cpp
// Route events to Your Window separate window
if (g_yourWindow != nullptr)
{
    g_yourWindow->ProcessEvent(event);
}

// Route events to other separate windows...
```

#### Add Hotkey (in SDL_AppEvent)
```cpp
// FX toggles Your Window
if (event->key.key == SDLK_FX)  // Replace FX with your key
{
    if (g_yourWindow)
    {
        g_yourWindow->Toggle();
        SYSTEM_LOG << "Your Window " 
                  << (g_yourWindow->IsOpen() ? "opened" : "closed") 
                  << endl;
    }
    return SDL_APP_CONTINUE;
}
```

#### Add Update Call (in SDL_AppIterate, AFTER main window rendering)
```cpp
// Update Your Window (separate window)
if (g_yourWindow)
{
    g_yourWindow->Update(GameEngine::fDt);
}
```

#### Add Cleanup (in SDL_AppQuit)
```cpp
// Cleanup Your Window
if (g_yourWindow)
{
    delete g_yourWindow;
    g_yourWindow = nullptr;
}
```

## Important Considerations

### ImGui Context Management
**CRITICAL:** Always save and restore the main ImGui context when working with the separate context.

```cpp
// Save main context
ImGuiContext* mainContext = ImGui::GetCurrentContext();

// Switch to separate context
ImGui::SetCurrentContext(m_separateImGuiContext);

// Do work...

// Restore main context
ImGui::SetCurrentContext(mainContext);
```

**Why?** ImGui uses a global context pointer. If you don't restore it, the main window will try to render with the wrong context, causing crashes or corruption.

### Event Routing Order
Route events in this order in `SDL_AppEvent()`:
1. **Separate windows** (your window, BT debugger, etc.)
2. **Main window ImGui** (ImGui_ImplSDL3_ProcessEvent)
3. **Game logic** (input handling)

**Why?** Separate windows need first chance to handle their events. If main window processes first, events may leak between windows.

### Window Lifecycle
**Pattern used:**
- Window created on first Toggle()
- Window hidden (not destroyed) when closed
- Window destroyed only on app quit

**Why?** Creating/destroying windows is expensive. Hiding/showing is fast and preserves user's window position/size.

### Memory Management
Always check for null pointers:
```cpp
if (!m_separateWindow) return;
if (m_separateRenderer) { /* safe to use */ }
```

Call `DestroySeparateWindow()` in destructor to ensure cleanup.

### Window Flags
Common SDL window flags:
- `SDL_WINDOW_RESIZABLE` - User can resize
- `SDL_WINDOW_HIDDEN` - Start hidden (show with SDL_ShowWindow)
- `SDL_WINDOW_BORDERLESS` - No title bar/border
- `SDL_WINDOW_ALWAYS_ON_TOP` - Stay on top of other windows

### Background Color
Choose appropriate background color:
```cpp
SDL_SetRenderDrawColor(m_separateRenderer, R, G, B, 255);
```

Common choices:
- Dark gray: `(45, 45, 48)` - matches ImGui Dark theme
- Black: `(0, 0, 0)` - minimal
- Dark blue: `(18, 18, 20)` - BT Debugger uses this

## Common Pitfalls

### ❌ Pitfall 1: Forgetting to restore context
```cpp
// BAD - main context not restored
ImGui::SetCurrentContext(m_separateImGuiContext);
DoWork();
// Missing: ImGui::SetCurrentContext(mainContext);
```

**Result:** Main window crashes or renders incorrectly.

**Solution:** Always save and restore context.

### ❌ Pitfall 2: Not checking for null window
```cpp
// BAD - window might be null
SDL_GetWindowID(m_separateWindow);  // Crash if null!
```

**Result:** Crash when window not created.

**Solution:** Always check `if (m_separateWindow)` first.

### ❌ Pitfall 3: Processing events without window ID check
```cpp
// BAD - handles ALL window close events
if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
{
    Toggle(); // Wrong! This closes on ANY window close
}
```

**Result:** Your window closes when user closes a different window.

**Solution:** Check window ID:
```cpp
Uint32 windowID = SDL_GetWindowID(m_separateWindow);
if (event->window.windowID == windowID)
{
    Toggle(); // Correct!
}
```

### ❌ Pitfall 4: Destroying window in Toggle()
```cpp
// BAD - destroys window on close
if (!m_isOpen)
{
    DestroySeparateWindow(); // Too expensive!
}
```

**Result:** Slow to reopen, window position lost.

**Solution:** Just hide the window:
```cpp
if (!m_isOpen)
{
    SDL_HideWindow(m_separateWindow);
}
```

### ❌ Pitfall 5: Rendering in wrong order
```cpp
// BAD - rendering before ImGui::NewFrame()
ImGui::NewFrame();
SDL_RenderClear(m_separateRenderer);  // Wrong order!
```

**Result:** Black screen or rendering issues.

**Solution:** Clear BEFORE ImGui::NewFrame():
```cpp
SDL_RenderClear(m_separateRenderer);
ImGui::NewFrame();
// ... render UI ...
ImGui::Render();
```

## Testing Checklist

After implementing your standalone window:
- [ ] Window opens with hotkey
- [ ] Window closes with hotkey
- [ ] Window closes with X button
- [ ] Window can be moved independently
- [ ] Window can be resized
- [ ] Mouse input works correctly
- [ ] Keyboard input works correctly
- [ ] No crashes when opening/closing rapidly
- [ ] Other windows (Blueprint, BT Debugger) still work
- [ ] No memory leaks (run with Valgrind or ASAN)
- [ ] Clean shutdown when app quits

## Reference Implementations

**Good examples to study:**
- `Source/AI/BehaviorTreeDebugWindow.h/cpp` - Full-featured debugger
- `Source/Editor/AnimationEditorWindow.h/cpp` - Editor with panels

**Key methods to review:**
- `CreateSeparateWindow()` - Window creation
- `DestroySeparateWindow()` - Cleanup
- `RenderSeparateWindow()` - Rendering
- `ProcessEvent()` - Event handling
- `Toggle()` - Show/hide logic

## Advanced Topics

### Custom Window Icon
```cpp
// After creating window
SDL_Surface* icon = IMG_Load("Resources/icon.png");
if (icon)
{
    SDL_SetWindowIcon(m_separateWindow, icon);
    SDL_DestroySurface(icon);
}
```

### Window Position Persistence
```cpp
// Save position before closing
int x, y;
SDL_GetWindowPosition(m_separateWindow, &x, &y);
SaveToConfig(x, y);

// Restore position when creating
int x, y;
LoadFromConfig(&x, &y);
SDL_SetWindowPosition(m_separateWindow, x, y);
```

### Custom ImGui Font
```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("Resources/font.ttf", 16.0f);
```

### Multiple ImGui Viewports (Docking)
```cpp
// Enable docking in your ImGui context
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
```

## Summary

Creating a standalone window involves:
1. Add window/renderer/context members to your class
2. Implement Create/Destroy/Render/ProcessEvent methods
3. Update Toggle() to show/hide window
4. Call DestroySeparateWindow() in destructor
5. Integrate into OlympeEngine.cpp (events, update, cleanup)
6. Always manage ImGui context switching carefully
7. Test thoroughly!

Follow this pattern and your standalone window will integrate seamlessly with Olympe Engine's architecture.
