# Phase 2 Implementation Guide - Visual Blueprint Editor

## Overview

This guide provides a complete implementation roadmap for Phase 2 of the Olympe Blueprint Editor, which adds a visual node-based interface using ImGui and ImNodes.

## Prerequisites

### Required Libraries

1. **Dear ImGui** (v1.89+)
   - Source: https://github.com/ocornut/imgui
   - License: MIT
   - Size: ~500KB source code
   - Purpose: Immediate mode GUI framework

2. **ImNodes** (v0.5+)
   - Source: https://github.com/Nelarius/imnodes
   - License: MIT
   - Size: ~50KB source code
   - Purpose: Node editor extension for ImGui

3. **SDL3** (Already available)
   - Used for window management and rendering context
   - SDL3.dll already in project

### Directory Structure

```
OlympeBlueprintEditor/
├── third_party/
│   ├── imgui/              # Dear ImGui library
│   │   ├── imgui.h
│   │   ├── imgui.cpp
│   │   ├── imgui_draw.cpp
│   │   ├── imgui_widgets.cpp
│   │   ├── imgui_tables.cpp
│   │   ├── imgui_demo.cpp
│   │   └── backends/
│   │       ├── imgui_impl_sdl3.h
│   │       ├── imgui_impl_sdl3.cpp
│   │       ├── imgui_impl_sdlrenderer3.h
│   │       └── imgui_impl_sdlrenderer3.cpp
│   │
│   └── imnodes/            # ImNodes library
│       ├── imnodes.h
│       └── imnodes.cpp
│
├── src/
│   ├── main_gui.cpp        # NEW: GUI entry point
│   ├── BlueprintEditorGUI.cpp  # NEW: GUI implementation
│   ├── NodeEditorPanel.cpp     # NEW: Node editor
│   ├── PropertyPanel.cpp       # NEW: Properties panel
│   └── EntityListPanel.cpp     # NEW: Entity list
│
└── include/
    ├── BlueprintEditorGUI.h
    ├── NodeEditorPanel.h
    ├── PropertyPanel.h
    └── EntityListPanel.h
```

## Implementation Steps

### Step 1: Download and Integrate ImGui

```bash
# Download ImGui
cd OlympeBlueprintEditor
mkdir -p third_party/imgui
cd third_party/imgui

# Download from GitHub (or use git submodule)
wget https://github.com/ocornut/imgui/archive/refs/tags/v1.89.9.tar.gz
tar -xzf v1.89.9.tar.gz --strip-components=1

# We need these files:
# - imgui.h, imgui.cpp
# - imgui_draw.cpp, imgui_widgets.cpp, imgui_tables.cpp
# - imgui_internal.h
# - imconfig.h, imstb_*.h
# - backends/imgui_impl_sdl3.*, imgui_impl_sdlrenderer3.*
```

### Step 2: Download and Integrate ImNodes

```bash
cd ../
mkdir -p imnodes
cd imnodes

wget https://github.com/Nelarius/imnodes/archive/refs/tags/v0.5.tar.gz
tar -xzf v0.5.tar.gz --strip-components=1

# We need:
# - imnodes.h, imnodes.cpp
```

### Step 3: Update Makefile

```makefile
# OlympeBlueprintEditor/Makefile (Phase 2 version)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../Source -I../Source/third_party -Iinclude \
           -Ithird_party/imgui -Ithird_party/imgui/backends -Ithird_party/imnodes \
           `sdl3-config --cflags`

LDFLAGS = `sdl3-config --libs` -lGL

# ImGui sources
IMGUI_SRC = third_party/imgui/imgui.cpp \
            third_party/imgui/imgui_draw.cpp \
            third_party/imgui/imgui_widgets.cpp \
            third_party/imgui/imgui_tables.cpp \
            third_party/imgui/backends/imgui_impl_sdl3.cpp \
            third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp

# ImNodes sources
IMNODES_SRC = third_party/imnodes/imnodes.cpp

# Editor sources
EDITOR_SRC = src/main_gui.cpp \
             src/BlueprintEditorGUI.cpp \
             src/NodeEditorPanel.cpp \
             src/PropertyPanel.cpp \
             src/EntityListPanel.cpp \
             src/EntityBlueprint.cpp

TARGET_GUI = OlympeBlueprintEditorGUI

all: $(TARGET_GUI)

$(TARGET_GUI): $(IMGUI_SRC) $(IMNODES_SRC) $(EDITOR_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET_GUI)
```

### Step 4: Implement Main GUI Entry Point

Create `src/main_gui.cpp`:

```cpp
/*
 * Olympe Blueprint Editor - GUI Entry Point (Phase 2)
 */

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include "BlueprintEditorGUI.h"
#include <iostream>

int main(int argc, char** argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Olympe Blueprint Editor",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );
    
    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Create editor instance
    Olympe::BlueprintEditorGUI editor;
    editor.Initialize();

    // Main loop
    bool running = true;
    while (running)
    {
        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && 
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Render editor
        editor.Render();

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    editor.Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

### Step 5: Implement BlueprintEditorGUI Class

Create `include/BlueprintEditorGUI.h`:

```cpp
#pragma once

#include "EntityBlueprint.h"
#include <string>
#include <vector>

namespace Olympe
{
    class BlueprintEditorGUI
    {
    public:
        BlueprintEditorGUI();
        ~BlueprintEditorGUI();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        void RenderMenuBar();
        void RenderEntityList();
        void RenderNodeEditor();
        void RenderPropertyPanel();
        void RenderStatusBar();

        // File operations
        void NewBlueprint();
        void LoadBlueprint();
        void SaveBlueprint();
        void SaveBlueprintAs();

        // State
        Blueprint::EntityBlueprint m_CurrentBlueprint;
        std::string m_CurrentFilepath;
        bool m_HasUnsavedChanges;
        int m_SelectedComponentIndex;
        
        // UI state
        bool m_ShowDemoWindow;
        bool m_ShowFileDialog;
        char m_FilepathBuffer[256];
    };
}
```

Create `src/BlueprintEditorGUI.cpp`:

```cpp
#include "BlueprintEditorGUI.h"
#include <imgui.h>
#include <imnodes.h>
#include <fstream>

namespace Olympe
{
    BlueprintEditorGUI::BlueprintEditorGUI()
        : m_HasUnsavedChanges(false)
        , m_SelectedComponentIndex(-1)
        , m_ShowDemoWindow(false)
        , m_ShowFileDialog(false)
    {
        m_FilepathBuffer[0] = '\0';
    }

    BlueprintEditorGUI::~BlueprintEditorGUI()
    {
    }

    void BlueprintEditorGUI::Initialize()
    {
        // Initialize ImNodes
        ImNodes::CreateContext();
        ImNodes::StyleColorsDark();
    }

    void BlueprintEditorGUI::Shutdown()
    {
        ImNodes::DestroyContext();
    }

    void BlueprintEditorGUI::Render()
    {
        // Setup dockspace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        // DockSpace
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Render components
        RenderMenuBar();
        RenderEntityList();
        RenderNodeEditor();
        RenderPropertyPanel();
        RenderStatusBar();

        ImGui::End();

        // Demo window for testing
        if (m_ShowDemoWindow)
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);
    }

    void BlueprintEditorGUI::RenderMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewBlueprint();
                
                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    LoadBlueprint();
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Save", "Ctrl+S", false, !m_CurrentBlueprint.name.empty()))
                    SaveBlueprint();
                
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                    SaveBlueprintAs();
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                    ; // Handle exit
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Documentation"))
                    ; // Open docs
                if (ImGui::MenuItem("About"))
                    ; // Show about dialog
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void BlueprintEditorGUI::RenderEntityList()
    {
        ImGui::Begin("Entity Properties");

        // Blueprint name
        if (!m_CurrentBlueprint.name.empty())
        {
            ImGui::Text("Blueprint: %s", m_CurrentBlueprint.name.c_str());
            if (m_HasUnsavedChanges)
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
            
            ImGui::Separator();

            // Component list
            ImGui::Text("Components (%zu)", m_CurrentBlueprint.components.size());
            
            for (size_t i = 0; i < m_CurrentBlueprint.components.size(); ++i)
            {
                const auto& comp = m_CurrentBlueprint.components[i];
                bool selected = (m_SelectedComponentIndex == (int)i);
                
                if (ImGui::Selectable(comp.type.c_str(), selected))
                {
                    m_SelectedComponentIndex = (int)i;
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Add Component"))
            {
                // Show component selection dialog
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Remove") && m_SelectedComponentIndex >= 0)
            {
                // Remove selected component
                m_CurrentBlueprint.components.erase(
                    m_CurrentBlueprint.components.begin() + m_SelectedComponentIndex
                );
                m_SelectedComponentIndex = -1;
                m_HasUnsavedChanges = true;
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No blueprint loaded");
            ImGui::Text("Use File > New or File > Open");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderNodeEditor()
    {
        ImGui::Begin("Component Graph");

        if (!m_CurrentBlueprint.name.empty())
        {
            ImNodes::BeginNodeEditor();

            // Render each component as a node
            for (size_t i = 0; i < m_CurrentBlueprint.components.size(); ++i)
            {
                const auto& comp = m_CurrentBlueprint.components[i];
                
                ImNodes::BeginNode((int)i);
                
                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(comp.type.c_str());
                ImNodes::EndNodeTitleBar();
                
                // Node content
                ImGui::Text("Properties: %zu", comp.properties.size());
                
                ImNodes::EndNode();
            }

            ImNodes::EndNodeEditor();
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No blueprint loaded");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderPropertyPanel()
    {
        ImGui::Begin("Properties");

        if (m_SelectedComponentIndex >= 0 && 
            m_SelectedComponentIndex < (int)m_CurrentBlueprint.components.size())
        {
            auto& comp = m_CurrentBlueprint.components[m_SelectedComponentIndex];
            
            ImGui::Text("Component: %s", comp.type.c_str());
            ImGui::Separator();

            // Display properties (simplified)
            ImGui::Text("Properties:");
            ImGui::TextWrapped("%s", comp.properties.dump(2).c_str());

            // TODO: Add property editors based on type
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                "Select a component to edit properties");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::RenderStatusBar()
    {
        ImGui::Begin("Status", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        
        if (!m_CurrentBlueprint.name.empty())
        {
            ImGui::Text("Blueprint: %s", m_CurrentBlueprint.name.c_str());
            ImGui::SameLine();
            ImGui::Text(" | Components: %zu", m_CurrentBlueprint.components.size());
            
            if (m_HasUnsavedChanges)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "| Modified");
            }
        }
        else
        {
            ImGui::Text("Ready");
        }

        ImGui::End();
    }

    void BlueprintEditorGUI::NewBlueprint()
    {
        // TODO: Show new blueprint dialog
        m_CurrentBlueprint = Blueprint::EntityBlueprint("NewBlueprint");
        m_CurrentFilepath.clear();
        m_HasUnsavedChanges = true;
        m_SelectedComponentIndex = -1;
    }

    void BlueprintEditorGUI::LoadBlueprint()
    {
        // TODO: Show file open dialog
        // For now, hardcode a test file
        auto loaded = Blueprint::EntityBlueprint::LoadFromFile("Blueprints/example_entity_simple.json");
        if (!loaded.name.empty())
        {
            m_CurrentBlueprint = loaded;
            m_CurrentFilepath = "Blueprints/example_entity_simple.json";
            m_HasUnsavedChanges = false;
            m_SelectedComponentIndex = -1;
        }
    }

    void BlueprintEditorGUI::SaveBlueprint()
    {
        if (m_CurrentFilepath.empty())
        {
            SaveBlueprintAs();
            return;
        }

        if (m_CurrentBlueprint.SaveToFile(m_CurrentFilepath))
        {
            m_HasUnsavedChanges = false;
        }
    }

    void BlueprintEditorGUI::SaveBlueprintAs()
    {
        // TODO: Show file save dialog
        m_ShowFileDialog = true;
    }
}
```

### Step 6: Build and Test

```bash
# Install SDL3 development files if needed
sudo apt-get install libsdl3-dev

# Build
cd OlympeBlueprintEditor
make

# Run
./OlympeBlueprintEditorGUI
```

## Advanced Features

### Property Editors

Implement type-specific property editors in `PropertyPanel.cpp`:

```cpp
void RenderPropertyEditor(const std::string& key, json& value)
{
    if (value.is_number())
    {
        float val = value.get<float>();
        if (ImGui::DragFloat(key.c_str(), &val, 0.1f))
        {
            value = val;
            m_HasUnsavedChanges = true;
        }
    }
    else if (value.is_string())
    {
        std::string str = value.get<std::string>();
        char buffer[256];
        strncpy(buffer, str.c_str(), 255);
        buffer[255] = '\0';
        
        if (ImGui::InputText(key.c_str(), buffer, 256))
        {
            value = std::string(buffer);
            m_HasUnsavedChanges = true;
        }
    }
    else if (value.is_boolean())
    {
        bool val = value.get<bool>();
        if (ImGui::Checkbox(key.c_str(), &val))
        {
            value = val;
            m_HasUnsavedChanges = true;
        }
    }
    else if (value.is_object())
    {
        if (ImGui::TreeNode(key.c_str()))
        {
            for (auto& [k, v] : value.items())
            {
                RenderPropertyEditor(k, v);
            }
            ImGui::TreePop();
        }
    }
}
```

### Node Connections

For connecting nodes (later phases):

```cpp
// In RenderNodeEditor()
int link_id = 0;
for (const auto& connection : m_Connections)
{
    ImNodes::Link(link_id++, connection.from_node, connection.to_node);
}

// Handle new connections
int start_attr, end_attr;
if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
{
    m_Connections.push_back({start_attr, end_attr});
    m_HasUnsavedChanges = true;
}
```

## Testing

### Manual Testing Checklist

- [ ] Window opens and renders correctly
- [ ] Can create new blueprint
- [ ] Can load existing blueprint
- [ ] Component list displays correctly
- [ ] Can select components
- [ ] Property panel shows component properties
- [ ] Can edit properties
- [ ] Can save blueprint
- [ ] Status bar updates correctly

### Automated Testing

Create `tests/gui_test.cpp` for automated GUI testing using ImGui test engine.

## Performance Considerations

- Limit node count per blueprint (< 100 nodes)
- Use ImNodes minimap for large graphs
- Implement viewport culling for large blueprints
- Cache JSON parsing results

## Troubleshooting

### SDL3 not found
```bash
# Install SDL3
sudo apt-get install libsdl3-dev
# Or build from source
```

### ImGui compilation errors
- Check include paths in Makefile
- Ensure all ImGui source files are included
- Verify C++17 support

### ImNodes not rendering
- Ensure ImNodes::CreateContext() is called
- Check ImNodes::BeginNodeEditor() / EndNodeEditor() pairs
- Verify ImNodes is included after ImGui

## Next Steps

1. Implement file dialogs (use nativefiledialog or similar)
2. Add component palette for drag & drop
3. Implement behavior graph editor
4. Add undo/redo system
5. Create property binding system
6. Add real-time preview panel

## Resources

- [ImGui Documentation](https://github.com/ocornut/imgui)
- [ImNodes Examples](https://github.com/Nelarius/imnodes)
- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/FrontPage)

---

**Status**: Implementation guide complete  
**Estimated Time**: 1-2 weeks for full Phase 2  
**Complexity**: High (GUI programming, event handling, rendering)
