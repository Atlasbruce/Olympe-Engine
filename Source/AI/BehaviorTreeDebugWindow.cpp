/**
 * @file BehaviorTreeDebugWindow.cpp
 * @brief Implementation of behavior tree runtime debugger
 */

#include "BehaviorTreeDebugWindow.h"
#include "../World.h"
#include "../GameEngine.h"
#include "../ECS_Components.h"
#include "../ECS_Components_AI.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include "../third_party/nlohmann/json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fstream>
#include <unordered_set>

using json = nlohmann::json;

namespace Olympe
{
    // Camera zoom constants
    constexpr float MIN_ZOOM = 0.3f;
    constexpr float MAX_ZOOM = 3.0f;
    constexpr float ZOOM_EPSILON = 0.001f;  // Minimum zoom change to trigger layout recomputation

    BehaviorTreeDebugWindow::BehaviorTreeDebugWindow()
        : m_separateWindow(nullptr)          // C++14: explicit initialization
        , m_separateRenderer(nullptr)        // C++14: explicit initialization
        , m_windowCreated(false)
        , m_separateImGuiContext(nullptr)
    {
    }

    BehaviorTreeDebugWindow::~BehaviorTreeDebugWindow()
    {
        Shutdown();
    }

    void BehaviorTreeDebugWindow::Initialize()
    {
        if (m_isInitialized)
            return;

        // Initialize ImNodes context (for node graph rendering)
        if (!m_imnodesInitialized)
        {
            ImNodes::CreateContext();
            ImNodes::GetStyle().GridSpacing = 32.0f;
            ImNodes::GetStyle().NodeCornerRounding = 8.0f;
            ImNodes::GetStyle().NodePadding = ImVec2(8, 8);
            m_imnodesInitialized = true;
        }

        m_isInitialized = true;
        
        std::cout << "[BTDebugger] Initialized (window will be created on first F10)" << std::endl;
    }

    void BehaviorTreeDebugWindow::Shutdown()
    {
        // Destroy separate window if exists
        DestroySeparateWindow();
        
        if (m_imnodesInitialized)
        {
            ImNodes::DestroyContext();
            m_imnodesInitialized = false;
        }

        m_isInitialized = false;
    }

    void BehaviorTreeDebugWindow::ToggleVisibility()
    {
        m_isVisible = !m_isVisible;
        
        if (m_isVisible)
        {
            // Opening: Create separate window if not exists
            if (!m_isInitialized)
            {
                Initialize();
            }
            
            if (!m_windowCreated)
            {
                CreateSeparateWindow();
            }
            
            std::cout << "[BTDebugger] F10: Debugger window opened (separate window)" << std::endl;
        }
        else
        {
            // Closing: Destroy separate window
            DestroySeparateWindow();
            
            std::cout << "[BTDebugger] F10: Debugger window closed" << std::endl;
        }
    }
    
    void BehaviorTreeDebugWindow::CreateSeparateWindow()
    {
        if (m_windowCreated)
        {
            std::cout << "[BTDebugger] Separate window already exists" << std::endl;
            return;
        }
        
        // Save current ImGui context before switching
        ImGuiContext* previousContext = ImGui::GetCurrentContext();
        
        // Create native SDL3 window (NOT ImGui viewport)
        const int windowWidth = 1200;
        const int windowHeight = 720;
        
        if (!SDL_CreateWindowAndRenderer(
            "Behavior Tree Runtime Debugger - Independent Window",
            windowWidth,
            windowHeight,
            SDL_WINDOW_RESIZABLE,  // User can resize
            &m_separateWindow,
            &m_separateRenderer))
        {
            std::cout << "[BTDebugger] ERROR: Failed to create separate window: " 
                      << SDL_GetError() << std::endl;
            return;
        }
        
        // Create separate ImGui context for this window
        m_separateImGuiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_separateImGuiContext);
        
        // Initialize ImGui backends for separate window
        ImGuiIO& io = ImGui::GetIO();
        (void)io; // Prevent unused variable warning
        ImGui::StyleColorsDark();
        
        ImGui_ImplSDL3_InitForSDLRenderer(m_separateWindow, m_separateRenderer);
        ImGui_ImplSDLRenderer3_Init(m_separateRenderer);
        
        m_windowCreated = true;
        
        // Restore previous ImGui context
        ImGui::SetCurrentContext(previousContext);
        
        std::cout << "[BTDebugger] ✅ Separate window created successfully!" << std::endl;
        std::cout << "[BTDebugger] Window can be moved to second monitor" << std::endl;
    }
    
    void BehaviorTreeDebugWindow::DestroySeparateWindow()
    {
        if (!m_windowCreated)
            return;

        // ✅ Sauvegarder le contexte principal
        ImGuiContext* previousContext = ImGui::GetCurrentContext();

        if (m_separateImGuiContext != nullptr)
        {
            ImGui::SetCurrentContext(m_separateImGuiContext);
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(m_separateImGuiContext);
            m_separateImGuiContext = nullptr;
        }

        // ✅ Restaurer le contexte principal (seulement s'il n'était pas celui qu'on vient de détruire)
        if (previousContext != m_separateImGuiContext)
            ImGui::SetCurrentContext(previousContext);

        // Destroy SDL3 resources
        if (m_separateRenderer != nullptr)
        {
            SDL_DestroyRenderer(m_separateRenderer);
            m_separateRenderer = nullptr;
        }
        
        if (m_separateWindow != nullptr)
        {
            SDL_DestroyWindow(m_separateWindow);
            m_separateWindow = nullptr;
        }
        
        m_windowCreated = false;
        
        std::cout << "[BTDebugger] Separate window destroyed" << std::endl;
    }
    
    void BehaviorTreeDebugWindow::ProcessEvent(SDL_Event* event)
    {
        if (!m_windowCreated || !m_isVisible)
            return;
        
        // Only process events for our separate window
        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            // Check if it's our window
            if (event->window.windowID == SDL_GetWindowID(m_separateWindow))
            {
                ToggleVisibility();  // Close debugger
                return;
            }
        }

		// Save current ImGui context
        ImGuiContext* previousContext = ImGui::GetCurrentContext();
                
        // Switch to our ImGui context and process event
        ImGui::SetCurrentContext(m_separateImGuiContext);
        ImGui_ImplSDL3_ProcessEvent(event);

        // Restore previous ImGui context
        ImGui::SetCurrentContext(previousContext);
    }

    void BehaviorTreeDebugWindow::Render()
    {
        if (!m_isVisible || !m_windowCreated)
            return;

        // Save current ImGui context
        ImGuiContext* previousContext = ImGui::GetCurrentContext();
        
        // Switch to separate window's ImGui context
        ImGui::SetCurrentContext(m_separateImGuiContext);
        
        // Begin new frame for separate window
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        // Render debugger content (existing code, but now in separate window)
        RenderInSeparateWindow();
        
        // Render ImGui to separate window
        ImGui::Render();
        SDL_SetRenderDrawColor(m_separateRenderer, 18, 18, 20, 255);  // Dark background
        SDL_RenderClear(m_separateRenderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_separateRenderer);
        SDL_RenderPresent(m_separateRenderer);

        // Restore previous ImGui context
        ImGui::SetCurrentContext(previousContext);
    }
    
    void BehaviorTreeDebugWindow::RenderInSeparateWindow()
    {
        // Update animations and timers
        m_pulseTimer += GameEngine::fDt;
        
        // Auto-refresh entity list
        static float accumulatedTime = 0.0f;
        accumulatedTime += GameEngine::fDt;
        
        if (accumulatedTime >= m_autoRefreshInterval)
        {
            RefreshEntityList();
            accumulatedTime = 0.0f;
        }
        
        // Update execution log timers
        for (auto& entry : m_executionLog)
        {
            entry.timeAgo += GameEngine::fDt;
        }
        
        // Main window (fills entire separate window)
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        ImGuiWindowFlags windowFlags = 
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoTitleBar |     // No title bar (OS window has title)
            ImGuiWindowFlags_NoResize |       // Use OS window resize
            ImGuiWindowFlags_NoMove |         // Fills entire window
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        if (!ImGui::Begin("Behavior Tree Runtime Debugger##Main", nullptr, windowFlags))
        {
            ImGui::End();
            return;
        }
        
        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                ImGui::SliderFloat("Auto Refresh (s)", &m_autoRefreshInterval, 0.1f, 5.0f);
                ImGui::SliderFloat("Entity List Width", &m_entityListWidth, 150.0f, 400.0f);
                ImGui::SliderFloat("Inspector Width", &m_inspectorWidth, 250.0f, 500.0f);
                
                // Reduced ranges, mark for recomputation
                if (ImGui::SliderFloat("Node Spacing X", &m_nodeSpacingX, 80.0f, 400.0f))
                {
                    m_needsLayoutUpdate = true;
                }
                if (ImGui::SliderFloat("Node Spacing Y", &m_nodeSpacingY, 60.0f, 300.0f))
                {
                    m_needsLayoutUpdate = true;
                }
                
                // Reset button to restore defaults
                if (ImGui::Button("Reset Spacing to Defaults"))
                {
                    m_nodeSpacingX = 180.0f;
                    m_nodeSpacingY = 120.0f;
                    m_needsLayoutUpdate = true;
                }
                
                ImGui::Separator();
                ImGui::Text("Current Zoom: %.0f%%", m_currentZoom * 100.0f);
                ImGui::Checkbox("Show Minimap", &m_showMinimap);
                
                // Auto-fit option
                ImGui::Checkbox("Auto-Fit on Load", &m_autoFitOnLoad);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Automatically fit tree to view when selecting an entity");
                }
                
                ImGui::Separator();
                ImGui::Text("Window Mode: Separate (Independent)");
                ImGui::Text("Press F10 to close window");
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Actions"))
            {
                if (ImGui::MenuItem("Refresh Now (F5)"))
                {
                    RefreshEntityList();
                }
                if (ImGui::MenuItem("Clear Execution Log"))
                {
                    m_executionLog.clear();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Fit Graph to View", "F"))
                    FitGraphToView();
                
                if (ImGui::MenuItem("Center View", "C"))
                    CenterViewOnGraph();
                
                if (ImGui::MenuItem("Reset Zoom", "0"))
                    ResetZoom();
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Keyboard shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            RefreshEntityList();
        }
        
        // Three-panel layout
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float windowHeight = ImGui::GetContentRegionAvail().y;
        
        // Left panel: Entity list
        ImGui::BeginChild("EntityListPanel", ImVec2(m_entityListWidth, windowHeight), true);
        RenderEntityListPanel();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Center panel: Node graph
        float centerWidth = windowWidth - m_entityListWidth - m_inspectorWidth - 20.0f;
        ImGui::BeginChild("NodeGraphPanel", ImVec2(centerWidth, windowHeight), true);
        RenderNodeGraphPanel();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right panel: Inspector
        ImGui::BeginChild("InspectorPanel", ImVec2(m_inspectorWidth, windowHeight), true);
        RenderInspectorPanel();
        ImGui::EndChild();
        
        ImGui::End();
    }

    void BehaviorTreeDebugWindow::RefreshEntityList()
    {
        m_entities.clear();

        // Query all entities with BehaviorTreeRuntime_data
        auto& world = World::Get();
        const auto& allEntities = world.GetAllEntities();

        for (EntityID entity : allEntities)
        {
            if (!world.HasComponent<BehaviorTreeRuntime_data>(entity))
                continue;

            EntityDebugInfo info;
            info.entityId = entity;

            // Get entity name
            if (world.HasComponent<Identity_data>(entity))
            {
                const auto& identity = world.GetComponent<Identity_data>(entity);
                info.entityName = identity.name;
            }
            else
            {
                info.entityName = "Entity " + std::to_string(entity);
            }

            // Get BT runtime data
            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(entity);
            info.treeId = btRuntime.AITreeAssetId;
            info.isActive = btRuntime.isActive;
            info.currentNodeId = btRuntime.AICurrentNodeIndex;
            info.lastStatus = static_cast<BTStatus>(btRuntime.lastStatus);

            // Get tree name - FIXED: Use enhanced lookup
            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
            if (tree)
            {
                info.treeName = tree->name;
            }
            else
            {
                // Fallback: Try to get path from registry
                std::string path = BehaviorTreeManager::Get().GetTreePathFromId(info.treeId);
                
                if (!path.empty())
                {
                    info.treeName = "Not Loaded: " + path;
                }
                else
                {
                    info.treeName = "Unknown (ID=" + std::to_string(info.treeId) + ")";
                }
                
                // DEBUG: Print diagnostic info (only once per entity)
                // Note: Static set persists for program lifetime. This is acceptable for
                // a debug tool as: 1) Debugger UI runs on main thread only (ImGui is single-threaded),
                // 2) Typical games have <1000 entities (~8KB memory), 3) Debug tools are used during
                // development, not in production. If unbounded growth becomes an issue, clear on
                // level change or add max size limit.
                static std::set<EntityID> debuggedEntities;
                if (debuggedEntities.find(entity) == debuggedEntities.end())
                {
                    debuggedEntities.insert(entity);
                    std::cout << "[BTDebugger] WARNING: Entity " << entity << " (" << info.entityName 
                              << ") has unknown tree ID=" << info.treeId << std::endl;
                    BehaviorTreeManager::Get().DebugPrintLoadedTrees();
                }
            }

            // Get AI state
            if (world.HasComponent<AIState_data>(entity))
            {
                const auto& aiState = world.GetComponent<AIState_data>(entity);
                switch (aiState.currentMode)
                {
                    case AIMode::Idle: info.aiMode = "Idle"; break;
                    case AIMode::Patrol: info.aiMode = "Patrol"; break;
                    case AIMode::Combat: info.aiMode = "Combat"; break;
                    case AIMode::Flee: info.aiMode = "Flee"; break;
                    case AIMode::Investigate: info.aiMode = "Investigate"; break;
                    case AIMode::Dead: info.aiMode = "Dead"; break;
                    default: info.aiMode = "Unknown"; break;
                }
            }
            else
            {
                info.aiMode = "N/A";
            }

            // Check if has target
            if (world.HasComponent<AIBlackboard_data>(entity))
            {
                const auto& blackboard = world.GetComponent<AIBlackboard_data>(entity);
                info.hasTarget = blackboard.hasTarget;
            }

            // Use accumulated game time if available, otherwise leave as 0
            // Note: This field is not currently used for sorting by time
            info.lastUpdateTime = 0.0f;

            m_entities.push_back(info);
        }

        // Apply filtering and sorting
        UpdateEntityFiltering();
        UpdateEntitySorting();
    }

    void BehaviorTreeDebugWindow::UpdateEntityFiltering()
    {
        m_filteredEntities.clear();

        for (const auto& info : m_entities)
        {
            // Apply text filter
            if (m_filterText[0] != '\0')
            {
                std::string lowerName = info.entityName;
                std::string lowerFilter = m_filterText;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
                
                if (lowerName.find(lowerFilter) == std::string::npos)
                    continue;
            }

            // Apply active only filter
            if (m_filterActiveOnly && !info.isActive)
                continue;

            // Apply has target filter
            if (m_filterHasTarget && !info.hasTarget)
                continue;

            m_filteredEntities.push_back(info);
        }
    }

    void BehaviorTreeDebugWindow::UpdateEntitySorting()
    {
        std::sort(m_filteredEntities.begin(), m_filteredEntities.end(), 
            [this](const EntityDebugInfo& a, const EntityDebugInfo& b) {
                bool result = false;
                switch (m_sortMode)
                {
                    case SortMode::Name:
                        result = a.entityName < b.entityName;
                        break;
                    case SortMode::TreeName:
                        result = a.treeName < b.treeName;
                        break;
                    case SortMode::LastUpdate:
                        result = a.lastUpdateTime > b.lastUpdateTime;
                        break;
                    case SortMode::AIMode:
                        result = a.aiMode < b.aiMode;
                        break;
                }
                return m_sortAscending ? result : !result;
            });
    }

    void BehaviorTreeDebugWindow::RenderEntityListPanel()
    {
        ImGui::Text("Entities with Behavior Trees");
        ImGui::Separator();

        // Filtering section
        ImGui::InputText("Search", m_filterText, sizeof(m_filterText));
        if (ImGui::IsItemEdited())
        {
            UpdateEntityFiltering();
            UpdateEntitySorting();
        }

        if (ImGui::Checkbox("Active Only", &m_filterActiveOnly))
        {
            UpdateEntityFiltering();
            UpdateEntitySorting();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Has Target", &m_filterHasTarget))
        {
            UpdateEntityFiltering();
            UpdateEntitySorting();
        }

        ImGui::Separator();

        // Sorting section
        ImGui::Text("Sort by:");
        const char* sortModes[] = { "Name", "Tree Name", "Last Update", "AI Mode" };
        int currentSort = static_cast<int>(m_sortMode);
        if (ImGui::Combo("##SortMode", &currentSort, sortModes, IM_ARRAYSIZE(sortModes)))
        {
            m_sortMode = static_cast<SortMode>(currentSort);
            UpdateEntitySorting();
        }
        ImGui::SameLine();
        if (ImGui::Button(m_sortAscending ? "Asc" : "Desc"))
        {
            m_sortAscending = !m_sortAscending;
            UpdateEntitySorting();
        }

        ImGui::Separator();

        // Entity list
        ImGui::Text("Entities: %d / %d", (int)m_filteredEntities.size(), (int)m_entities.size());

        ImGui::BeginChild("EntityList", ImVec2(0, 0), false);
        for (const auto& info : m_filteredEntities)
        {
            RenderEntityEntry(info);
        }
        ImGui::EndChild();
    }

    void BehaviorTreeDebugWindow::RenderEntityEntry(const EntityDebugInfo& info)
    {
        ImGui::PushID(info.entityId);

        // Status icon
        const char* statusIcon = info.isActive ? "●" : "○";
        ImVec4 statusColor = info.isActive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        
        // Status result icon
        const char* resultIcon = "▶";
        ImVec4 resultColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Running
        if (info.lastStatus == BTStatus::Success)
        {
            resultIcon = "✓";
            resultColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        else if (info.lastStatus == BTStatus::Failure)
        {
            resultIcon = "✗";
            resultColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        }

        // Selectable
        bool isSelected = (m_selectedEntity == info.entityId);
        ImGui::TextColored(statusColor, "%s", statusIcon);
        ImGui::SameLine();
        ImGui::TextColored(resultColor, "%s", resultIcon);
        ImGui::SameLine();

        if (ImGui::Selectable(info.entityName.c_str(), isSelected))
        {
            m_selectedEntity = info.entityId;
            
            // Recompute layout for selected entity - FIXED: Use enhanced lookup
            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
            if (tree)
            {
                // Pass current zoom factor
                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                m_needsLayoutUpdate = false;
                
                // Auto-fit if enabled
                if (m_autoFitOnLoad)
                {
                    // Defer fit to next frame by clearing the last centered entity
                    // Note: We rely on the fact that entity selection changed, so m_selectedEntity != m_lastCenteredEntity
                    // This will trigger the auto-center/fit logic in the graph panel
                }
            }
        }

        // Tooltip
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Entity ID: %u", info.entityId);
            ImGui::Text("Tree: %s", info.treeName.c_str());
            ImGui::Text("AI Mode: %s", info.aiMode.c_str());
            ImGui::Text("Active: %s", info.isActive ? "Yes" : "No");
            ImGui::Text("Has Target: %s", info.hasTarget ? "Yes" : "No");
            ImGui::EndTooltip();
        }

        ImGui::Indent();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", info.treeName.c_str());
        ImGui::Unindent();

        ImGui::PopID();
    }

    void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
    {
        if (m_selectedEntity == 0)
        {
            ImGui::Text("Select an entity from the list to view its behavior tree");
            return;
        }

        // Get selected entity's BT data
        auto& world = World::Get();
        if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
        {
            ImGui::Text("Selected entity no longer has a behavior tree");
            m_selectedEntity = 0;
            return;
        }

        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
        
        // FIXED: Use enhanced lookup
        const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

        if (!tree)
        {
            // More helpful error message
            std::string path = BehaviorTreeManager::Get().GetTreePathFromId(btRuntime.AITreeAssetId);
            
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Behavior Tree asset not found!");
            ImGui::Separator();
            ImGui::Text("Tree ID: %u", btRuntime.AITreeAssetId);
            
            if (!path.empty())
            {
                ImGui::Text("Expected Path: %s", path.c_str());
                ImGui::Spacing();
                ImGui::TextWrapped("The tree file may not be loaded. Check if the JSON file exists and is loaded during level initialization.");
            }
            else
            {
                ImGui::Spacing();
                ImGui::TextWrapped("This tree ID is not registered in the BehaviorTreeManager.");
                ImGui::TextWrapped("Possible causes:");
                ImGui::BulletText("Tree JSON file not loaded");
                ImGui::BulletText("Prefab uses obsolete tree ID");
                ImGui::BulletText("Tree ID mismatch between prefab and runtime");
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Show All Loaded Trees"))
            {
                BehaviorTreeManager::Get().DebugPrintLoadedTrees();
            }
            
            return;
        }

        // Editor mode toggle
        bool prevEditorMode = m_editorMode;
        ImGui::Checkbox("Editor Mode", &m_editorMode);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Enable editing mode to add/remove/connect nodes");
        }
        
        // Initialize editing tree when entering editor mode
        if (m_editorMode && !prevEditorMode)
        {
            auto& world = World::Get();
            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
            const BehaviorTreeAsset* originalTree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
            
            if (originalTree)
            {
                m_editingTree = *originalTree;
                m_treeModified = false;
                m_selectedNodes.clear();
                m_undoStack.clear();
                m_redoStack.clear();
                m_nextNodeId = 1000;
                m_nextLinkId = 100000;  // Reset link ID counter to prevent overflow
                
                // Find max node ID to avoid conflicts
                for (const auto& node : m_editingTree.nodes)
                {
                    if (node.id >= m_nextNodeId)
                    {
                        m_nextNodeId = node.id + 1;
                    }
                }
                
                std::cout << "[BTEditor] Entered editor mode, editing tree: " << m_editingTree.name << std::endl;
            }
        }
        
        if (m_editorMode)
        {
            ImGui::SameLine();
            if (m_treeModified)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "[Modified]");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Unmodified]");
            }
            
            // Editor toolbar
            RenderEditorToolbar();
        }
        
        ImGui::Separator();
        
        // Layout direction toggle
        ImGui::Text("Layout:");
        ImGui::SameLine();
        
        bool layoutChanged = false;
        if (ImGui::RadioButton("Vertical", m_layoutDirection == BTLayoutDirection::TopToBottom))
        {
            if (m_layoutDirection != BTLayoutDirection::TopToBottom)
            {
                m_layoutDirection = BTLayoutDirection::TopToBottom;
                layoutChanged = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Horizontal", m_layoutDirection == BTLayoutDirection::LeftToRight))
        {
            if (m_layoutDirection != BTLayoutDirection::LeftToRight)
            {
                m_layoutDirection = BTLayoutDirection::LeftToRight;
                layoutChanged = true;
            }
        }
        
        // Update layout engine and recompute if changed
        if (layoutChanged)
        {
            m_layoutEngine.SetLayoutDirection(m_layoutDirection);
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            m_needsLayoutUpdate = false;
        }
        
        // Recompute layout if spacing changed via sliders
        if (m_needsLayoutUpdate && tree)
        {
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            m_needsLayoutUpdate = false;
        }
        
        // Reset Camera button
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera"))
        {
            // Center camera on graph bounding box
            if (!m_currentLayout.empty())
            {
                ImVec2 minPos(FLT_MAX, FLT_MAX);
                ImVec2 maxPos(-FLT_MAX, -FLT_MAX);

                for (const auto& layout : m_currentLayout)
                {
                    float halfWidth = layout.width / 2.0f;
                    float halfHeight = layout.height / 2.0f;
                    
                    minPos.x = std::min(minPos.x, layout.position.x - halfWidth);
                    minPos.y = std::min(minPos.y, layout.position.y - halfHeight);
                    maxPos.x = std::max(maxPos.x, layout.position.x + halfWidth);
                    maxPos.y = std::max(maxPos.y, layout.position.y + halfHeight);
                }

                ImVec2 graphCenter((minPos.x + maxPos.x) / 2.0f, (minPos.y + maxPos.y) / 2.0f);
                ImVec2 editorSize = ImGui::GetContentRegionAvail();
                
                // Center camera on graph (panning = where graph origin is in screen space)
                ImVec2 targetPanning(
                    graphCenter.x - editorSize.x / 2.0f,
                    graphCenter.y - editorSize.y / 2.0f
                );
                
                ImNodes::EditorContextResetPanning(targetPanning);
                
                std::cout << "[BTDebugger] Camera reset to center: (" 
                          << graphCenter.x << ", " << graphCenter.y << ")" << std::endl;
            }
        }
        
        ImGui::Separator();

        // ImNodes editor
        ImNodes::BeginNodeEditor();

        // ✅ NEW: Calculate graph bounding box for camera centering
        if (!m_currentLayout.empty())
        {
            ImVec2 minPos(FLT_MAX, FLT_MAX);
            ImVec2 maxPos(-FLT_MAX, -FLT_MAX);

            for (const auto& layout : m_currentLayout)
            {
                float halfWidth = layout.width / 2.0f;
                float halfHeight = layout.height / 2.0f;
                
                minPos.x = std::min(minPos.x, layout.position.x - halfWidth);
                minPos.y = std::min(minPos.y, layout.position.y - halfHeight);
                maxPos.x = std::max(maxPos.x, layout.position.x + halfWidth);
                maxPos.y = std::max(maxPos.y, layout.position.y + halfHeight);
            }

            ImVec2 graphCenter((minPos.x + maxPos.x) / 2.0f, (minPos.y + maxPos.y) / 2.0f);
            ImVec2 graphSize(maxPos.x - minPos.x, maxPos.y - minPos.y);

            //std::cout << "[BTDebugger] Graph bounds: (" << minPos.x << "," << minPos.y << ") to (" << maxPos.x << "," << maxPos.y << ")" << std::endl;
            //std::cout << "[BTDebugger] Graph size: " << graphSize.x << "x" << graphSize.y << " pixels" << std::endl;
            //std::cout << "[BTDebugger] Graph center: (" << graphCenter.x << "," << graphCenter.y << ")" << std::endl;

            // Center camera when entity changes (with optional auto-fit)
            if (m_lastCenteredEntity != m_selectedEntity)
            {
                // Initialize editing tree when entity changes in editor mode
                if (m_editorMode)
                {
                    auto& world = World::Get();
                    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
                    const BehaviorTreeAsset* originalTree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
                    
                    if (originalTree)
                    {
                        m_editingTree = *originalTree;
                        m_treeModified = false;
                        m_selectedNodes.clear();
                        m_undoStack.clear();
                        m_redoStack.clear();
                    }
                }
                
                if (m_autoFitOnLoad)
                {
                    // Fit entire tree to view
                    FitGraphToView();
                }
                else
                {
                    // Just center without changing zoom
                    CenterViewOnGraph();
                }
                
                std::cout << "[BTDebugger] ✅ Camera " << (m_autoFitOnLoad ? "fitted" : "centered") << " on graph" << std::endl;
                m_lastCenteredEntity = m_selectedEntity;
            }
        }

        // Mouse wheel zoom with layout recomputation
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            if (io.MouseWheel != 0.0f)
            {
                float oldZoom = m_currentZoom;
                float zoomDelta = io.MouseWheel * 0.1f;  // 10% per wheel notch
                m_currentZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, m_currentZoom + zoomDelta));
                
                // Recompute layout with new zoom
                if (std::abs(m_currentZoom - oldZoom) > ZOOM_EPSILON && tree)
                {
                    m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                    ApplyZoomToStyle();  // Also update ImNodes style
                    
                    std::cout << "[BTDebugger] Zoom: " << (int)(m_currentZoom * 100) 
                              << "% (layout recomputed)" << std::endl;
                }
            }
        }

        // Keyboard shortcuts for camera control
        if (ImGui::IsWindowFocused())
        {
            bool ctrlPressed = io.KeyCtrl;
            
            // F : Fit to view
            if (ImGui::IsKeyPressed(ImGuiKey_F) && !ctrlPressed)
                FitGraphToView();
            
            // C : Center view
            if (ImGui::IsKeyPressed(ImGuiKey_C) && !ctrlPressed)
                CenterViewOnGraph();
            
            // 0 (numpad or main) : Reset zoom
            if ((ImGui::IsKeyPressed(ImGuiKey_0) || ImGui::IsKeyPressed(ImGuiKey_Keypad0)) && !ctrlPressed)
                ResetZoom();
            
            // M : Toggle minimap
            if (ImGui::IsKeyPressed(ImGuiKey_M) && !ctrlPressed)
                m_showMinimap = !m_showMinimap;
            
            // + / - : Zoom in/out (Note: '+' requires Shift+Equal on most keyboards)
            if ((ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) && !ctrlPressed)
            {
                float oldZoom = m_currentZoom;
                m_currentZoom = std::min(MAX_ZOOM, m_currentZoom * 1.2f);
                
                // Recompute layout
                if (std::abs(m_currentZoom - oldZoom) > ZOOM_EPSILON && tree)
                {
                    m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                    ApplyZoomToStyle();
                }
            }
            
            if ((ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) && !ctrlPressed)
            {
                float oldZoom = m_currentZoom;
                m_currentZoom = std::max(MIN_ZOOM, m_currentZoom / 1.2f);
                
                // Recompute layout
                if (std::abs(m_currentZoom - oldZoom) > ZOOM_EPSILON && tree)
                {
                    m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                    ApplyZoomToStyle();
                }
            }
        }

        RenderBehaviorTreeGraph();

        // Render minimap after the graph
        if (m_showMinimap)
            RenderMinimap();

        // Editor mode interactions (after graph rendering, before EndNodeEditor)
        if (m_editorMode)
        {
            // Detect link creation
            int startAttrId, endAttrId;
            if (ImNodes::IsLinkCreated(&startAttrId, &endAttrId))
            {
                // Convert attribute IDs back to node IDs
                uint32_t parentId = startAttrId / 10000;
                uint32_t childId = endAttrId / 10000;
                
                if (ValidateConnection(parentId, childId))
                {
                    // Find parent node and add connection
                    BTNode* parent = m_editingTree.GetNode(parentId);
                    if (parent)
                    {
                        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                        {
                            // Capture the actual index before adding the child
                            int childIndex = parent->childIds.size();
                            parent->childIds.push_back(childId);
                            
                            // Add to undo stack with correct index
                            EditorAction action;
                            action.type = EditorAction::AddConnection;
                            action.parentId = parentId;
                            action.childId = childId;
                            action.childIndex = childIndex;
                            m_undoStack.push_back(action);
                            if (m_undoStack.size() > kMaxUndoStackSize)
                            {
                                m_undoStack.erase(m_undoStack.begin());
                            }
                            m_redoStack.clear();
                            m_treeModified = true;
                        }
                        else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                        {
                            // Set decoratorChildId
                            parent->decoratorChildId = childId;
                            
                            // Add to undo stack
                            EditorAction action;
                            action.type = EditorAction::AddConnection;
                            action.parentId = parentId;
                            action.childId = childId;
                            action.childIndex = 0;  // Decorators don't use childIndex
                            m_undoStack.push_back(action);
                            if (m_undoStack.size() > kMaxUndoStackSize)
                            {
                                m_undoStack.erase(m_undoStack.begin());
                            }
                            m_redoStack.clear();
                            m_treeModified = true;
                        }
                        
                        std::cout << "[BTEditor] Connection created: " << parentId << " -> " << childId << std::endl;
                    }
                }
                else
                {
                    std::cout << "[BTEditor] Invalid connection: " << parentId << " -> " << childId << std::endl;
                }
            }
            
            // Detect link destruction
            int linkId;
            if (ImNodes::IsLinkDestroyed(&linkId))
            {
                // Find the link in our map
                auto it = std::find_if(m_linkMap.begin(), m_linkMap.end(),
                    [linkId](const LinkInfo& info) { return info.linkId == linkId; });
                
                if (it != m_linkMap.end())
                {
                    uint32_t parentId = it->parentId;
                    uint32_t childId = it->childId;
                    
                    // Remove the connection from the tree
                    BTNode* parent = m_editingTree.GetNode(parentId);
                    if (parent)
                    {
                        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                        {
                            auto childIt = std::find(parent->childIds.begin(), parent->childIds.end(), childId);
                            if (childIt != parent->childIds.end())
                            {
                                int childIndex = std::distance(parent->childIds.begin(), childIt);
                                parent->childIds.erase(childIt);
                                
                                // Add to undo stack
                                EditorAction action;
                                action.type = EditorAction::DeleteConnection;
                                action.parentId = parentId;
                                action.childId = childId;
                                action.childIndex = childIndex;
                                m_undoStack.push_back(action);
                                if (m_undoStack.size() > kMaxUndoStackSize)
                                {
                                    m_undoStack.erase(m_undoStack.begin());
                                }
                                m_redoStack.clear();
                            }
                        }
                        else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                        {
                            parent->decoratorChildId = 0;
                            
                            // Add to undo stack
                            EditorAction action;
                            action.type = EditorAction::DeleteConnection;
                            action.parentId = parentId;
                            action.childId = childId;
                            m_undoStack.push_back(action);
                            if (m_undoStack.size() > kMaxUndoStackSize)
                            {
                                m_undoStack.erase(m_undoStack.begin());
                            }
                            m_redoStack.clear();
                        }
                        
                        m_treeModified = true;
                        std::cout << "[BTEditor] Connection deleted: " << parentId << " -> " << childId << std::endl;
                    }
                }
            }
            
            // Detect node selection
            int numSelected = ImNodes::NumSelectedNodes();
            if (numSelected > 0)
            {
                std::vector<int> selectedIds(numSelected);
                ImNodes::GetSelectedNodes(selectedIds.data());
                m_selectedNodes.clear();
                for (int id : selectedIds)
                {
                    m_selectedNodes.push_back(static_cast<uint32_t>(id));
                }
            }
            
            // Right-click context menu
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                m_showNodePalette = true;
                m_nodeCreationPos = ImGui::GetMousePos();
            }
            
            // Delete key to delete selected nodes
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_selectedNodes.empty())
            {
                HandleNodeDeletion();
            }
            
            // Ctrl+D to duplicate selected nodes
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && !m_selectedNodes.empty())
            {
                HandleNodeDuplication();
            }
            
            // Ctrl+Z for undo
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
            {
                UndoLastAction();
            }
            
            // Ctrl+Y or Ctrl+Shift+Z for redo
            if (ImGui::GetIO().KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || 
                (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
            {
                RedoLastAction();
            }
        }

        ImNodes::EndNodeEditor();
        
        // Node palette popup (outside of node editor)
        if (m_showNodePalette)
        {
            RenderNodePalette();
        }
    }

    void BehaviorTreeDebugWindow::RenderBehaviorTreeGraph()
    {
        auto& world = World::Get();
        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
        
        // Use editing tree in editor mode, otherwise use runtime tree
        const BehaviorTreeAsset* tree = nullptr;
        
        if (m_editorMode && !m_editingTree.nodes.empty())
        {
            tree = &m_editingTree;
        }
        else
        {
            // FIXED: Use enhanced lookup
            tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
        }

        if (!tree)
            return;

        uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;

        // Render all nodes
        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                bool isCurrentNode = (node.id == currentNodeId) && btRuntime.isActive && !m_editorMode;
                RenderNode(&node, layout, isCurrentNode);
            }
        }

        // Render connections
        int linkId = 0;
        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                RenderNodeConnections(&node, layout, tree);
            }
        }
    }

    void BehaviorTreeDebugWindow::RenderNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode)
    {
        // Static local variable to track which nodes have been debug printed
        static std::unordered_set<uint32_t> printedNodeIds;
        
        if (!node || !layout)
            return;

        // ✅ NEW: Debug position (only print once per entity)
        if (printedNodeIds.find(node->id) == printedNodeIds.end())
        {
            std::cout << "[RenderNode] Node " << node->id 
                      << " (" << node->name << ") at (" 
                      << (int)layout->position.x << ", " << (int)layout->position.y << ")" << std::endl;
            printedNodeIds.insert(node->id);
        }

        // Set node position BEFORE BeginNode (ImNodes requirement)
        ImNodes::SetNodeGridSpacePos(node->id, ImVec2(layout->position.x, layout->position.y));

        ImNodes::BeginNode(node->id);

        // Node title bar
        ImNodes::BeginNodeTitleBar();
        
        // Get node color
        uint32_t color = GetNodeColor(node->type);
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, color);
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, color);
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, color);

        // Icon and name
        const char* icon = GetNodeIcon(node->type);
        ImGui::Text("%s %s", icon, node->name.c_str());

        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        
        ImNodes::EndNodeTitleBar();

        // Node body with generous width
        ImGui::PushItemWidth(200);  // Increased from 120 for better readability

        // Show node type
        const char* typeStr = "Unknown";
        switch (node->type)
        {
            case BTNodeType::Selector: typeStr = "Selector"; break;
            case BTNodeType::Sequence: typeStr = "Sequence"; break;
            case BTNodeType::Condition: typeStr = "Condition"; break;
            case BTNodeType::Action: typeStr = "Action"; break;
            case BTNodeType::Inverter: typeStr = "Inverter"; break;
            case BTNodeType::Repeater: typeStr = "Repeater"; break;
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Type: %s", typeStr);

        // Show ID
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ID: %u", node->id);
        
        // Add generous spacing
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::PopItemWidth();

        // Input/output attribute
        if (node->id != 0)  // Not root
        {
            ImNodes::BeginInputAttribute(node->id * 10000);
            ImGui::Text("In");
            ImNodes::EndInputAttribute();
        }

        // Output attribute for composite/decorator nodes
        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence ||
            node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater)
        {
            ImNodes::BeginOutputAttribute(node->id * 10000 + 1);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();
        }

        // Highlight current node with pulsing effect (BEFORE EndNode)
        if (isCurrentNode)
        {
            // Pulse between 0.5 and 1.0 alpha using sine wave
            float pulse = 0.5f + 0.5f * sinf(m_pulseTimer * 2.0f * 3.14159265f);  // 1 Hz pulse (2π radians/sec)
            uint32_t highlightColor = IM_COL32(255, 255, 0, static_cast<int>(pulse * 255));
            ImNodes::PushColorStyle(ImNodesCol_NodeOutline, highlightColor);
        }

        ImNodes::EndNode();
        
        // Pop color style after EndNode
        if (isCurrentNode)
        {
            ImNodes::PopColorStyle();
        }

        // Set node position
        ImNodes::SetNodeGridSpacePos(node->id, ImVec2(layout->position.x, layout->position.y));
    }

    void BehaviorTreeDebugWindow::RenderNodeConnections(const BTNode* node, const BTNodeLayout* layout, const BehaviorTreeAsset* tree)
    {
        // Clear link map at the start of each render pass (only in editor mode)
        // Use a frame counter to ensure we clear once per frame, not once per lifetime
        static int lastFrameCleared = -1;
        static int currentFrame = 0;
        currentFrame++;
        
        if (m_editorMode && lastFrameCleared != currentFrame)
        {
            m_linkMap.clear();
            lastFrameCleared = currentFrame;
        }
        
        // Composite nodes
        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence)
        {
            for (uint32_t childId : node->childIds)
            {
                int linkId = m_nextLinkId++;
                ImNodes::Link(linkId, node->id * 10000 + 1, childId * 10000);
                
                // Track this link in editor mode
                if (m_editorMode)
                {
                    LinkInfo info;
                    info.linkId = linkId;
                    info.parentId = node->id;
                    info.childId = childId;
                    m_linkMap.push_back(info);
                }
            }
        }
        // Decorator nodes
        else if ((node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater) && 
                  node->decoratorChildId != 0)
        {
            int linkId = m_nextLinkId++;
            ImNodes::Link(linkId, node->id * 10000 + 1, node->decoratorChildId * 10000);
            
            // Track this link in editor mode
            if (m_editorMode)
            {
                LinkInfo info;
                info.linkId = linkId;
                info.parentId = node->id;
                info.childId = node->decoratorChildId;
                m_linkMap.push_back(info);
            }
        }
    }

    uint32_t BehaviorTreeDebugWindow::GetNodeColor(BTNodeType type) const
    {
        switch (type)
        {
            case BTNodeType::Selector:
                return IM_COL32(100, 150, 255, 255);  // Blue
            case BTNodeType::Sequence:
                return IM_COL32(100, 255, 150, 255);  // Green
            case BTNodeType::Condition:
                return IM_COL32(255, 200, 100, 255);  // Orange
            case BTNodeType::Action:
                return IM_COL32(255, 100, 150, 255);  // Rose
            case BTNodeType::Inverter:
                return IM_COL32(200, 100, 255, 255);  // Purple
            case BTNodeType::Repeater:
                return IM_COL32(150, 150, 255, 255);  // Light Blue
            default:
                return IM_COL32(128, 128, 128, 255);  // Gray
        }
    }

    const char* BehaviorTreeDebugWindow::GetNodeIcon(BTNodeType type) const
    {
        switch (type)
        {
            case BTNodeType::Selector: return "?";
            case BTNodeType::Sequence: return "->";
            case BTNodeType::Condition: return "◆";
            case BTNodeType::Action: return "►";
            case BTNodeType::Inverter: return "!";
            case BTNodeType::Repeater: return "↻";
            default: return "•";
        }
    }

    void BehaviorTreeDebugWindow::RenderInspectorPanel()
    {
        if (m_selectedEntity == 0)
        {
            ImGui::Text("No entity selected");
            return;
        }

        ImGui::Text("Inspector");
        ImGui::Separator();

        // Runtime info
        if (ImGui::CollapsingHeader("Runtime Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderRuntimeInfo();
        }

        // Blackboard
        if (ImGui::CollapsingHeader("Blackboard", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderBlackboardSection();
        }

        // Execution log
        if (ImGui::CollapsingHeader("Execution Log", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderExecutionLog();
        }
    }

    void BehaviorTreeDebugWindow::RenderRuntimeInfo()
    {
        auto& world = World::Get();
        
        if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
            return;

        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
        
        // FIXED: Use enhanced lookup
        const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

        ImGui::Text("Tree ID: %u", btRuntime.AITreeAssetId);
        
        if (tree)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tree Name: %s", tree->name.c_str());
            ImGui::Text("Node Count: %zu", tree->nodes.size());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Tree: NOT FOUND");
            
            std::string path = BehaviorTreeManager::Get().GetTreePathFromId(btRuntime.AITreeAssetId);
            if (!path.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Expected: %s", path.c_str());
            }
            
            if (ImGui::Button("Debug: List All Trees"))
            {
                BehaviorTreeManager::Get().DebugPrintLoadedTrees();
            }
        }
        
        ImGui::Separator();

        ImGui::Text("Current Node ID: %u", btRuntime.AICurrentNodeIndex);
        
        if (tree)
        {
            const BTNode* currentNode = tree->GetNode(btRuntime.AICurrentNodeIndex);
            if (currentNode)
            {
                ImGui::Text("Node Name: %s", currentNode->name.c_str());
            }
        }

        // Last status
        const char* statusStr = "Running";
        ImVec4 statusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        BTStatus status = static_cast<BTStatus>(btRuntime.lastStatus);
        if (status == BTStatus::Success)
        {
            statusStr = "Success";
            statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        else if (status == BTStatus::Failure)
        {
            statusStr = "Failure";
            statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        ImGui::TextColored(statusColor, "Last Status: %s", statusStr);

        ImGui::Text("Active: %s", btRuntime.isActive ? "Yes" : "No");

        // AI State
        if (world.HasComponent<AIState_data>(m_selectedEntity))
        {
            const auto& aiState = world.GetComponent<AIState_data>(m_selectedEntity);
            const char* modeStr = "Unknown";
            switch (aiState.currentMode)
            {
                case AIMode::Idle: modeStr = "Idle"; break;
                case AIMode::Patrol: modeStr = "Patrol"; break;
                case AIMode::Combat: modeStr = "Combat"; break;
                case AIMode::Flee: modeStr = "Flee"; break;
                case AIMode::Investigate: modeStr = "Investigate"; break;
                case AIMode::Dead: modeStr = "Dead"; break;
            }
            ImGui::Text("AI Mode: %s", modeStr);
            ImGui::Text("Time in Mode: %.2f s", aiState.timeInCurrentMode);
        }
    }

    void BehaviorTreeDebugWindow::RenderBlackboardSection()
    {
        auto& world = World::Get();
        
        if (!world.HasComponent<AIBlackboard_data>(m_selectedEntity))
        {
            ImGui::Text("No blackboard data");
            return;
        }

        const auto& blackboard = world.GetComponent<AIBlackboard_data>(m_selectedEntity);

        // Target tracking
        if (ImGui::TreeNode("Target"))
        {
            ImGui::Text("Has Target: %s", blackboard.hasTarget ? "Yes" : "No");
            ImGui::Text("Target Entity: %u", blackboard.targetEntity);
            ImGui::Text("Target Visible: %s", blackboard.targetVisible ? "Yes" : "No");
            ImGui::Text("Distance: %.2f", blackboard.distanceToTarget);
            ImGui::Text("Time Since Seen: %.2f s", blackboard.timeSinceTargetSeen);
            ImGui::Text("Last Known Pos: (%.1f, %.1f)", 
                blackboard.lastKnownTargetPosition.x, 
                blackboard.lastKnownTargetPosition.y);
            ImGui::TreePop();
        }

        // Movement
        if (ImGui::TreeNode("Movement"))
        {
            ImGui::Text("Has Move Goal: %s", blackboard.hasMoveGoal ? "Yes" : "No");
            ImGui::Text("Goal Position: (%.1f, %.1f)", 
                blackboard.moveGoal.x, 
                blackboard.moveGoal.y);
            ImGui::TreePop();
        }

        // Patrol
        if (ImGui::TreeNode("Patrol"))
        {
            ImGui::Text("Has Patrol Path: %s", blackboard.hasPatrolPath ? "Yes" : "No");
            ImGui::Text("Current Point: %d", blackboard.currentPatrolPoint);
            ImGui::Text("Point Count: %d", blackboard.patrolPointCount);
            ImGui::TreePop();
        }

        // Combat
        if (ImGui::TreeNode("Combat"))
        {
            ImGui::Text("Can Attack: %s", blackboard.canAttack ? "Yes" : "No");
            ImGui::Text("Attack Cooldown: %.2f s", blackboard.attackCooldown);
            
            // Calculate time since last attack if lastAttackTime is a timestamp
            // Note: If lastAttackTime is 0, display "Never"
            if (blackboard.lastAttackTime > 0.0f)
            {
                // Assuming there's a global game time available
                // For now, just display the raw value
                ImGui::Text("Last Attack Time: %.2f", blackboard.lastAttackTime);
            }
            else
            {
                ImGui::Text("Last Attack: Never");
            }
            
            ImGui::TreePop();
        }

        // Stimuli
        if (ImGui::TreeNode("Stimuli"))
        {
            ImGui::Text("Heard Noise: %s", blackboard.heardNoise ? "Yes" : "No");
            ImGui::Text("Last Damage: %.2f", blackboard.damageAmount);
            ImGui::TreePop();
        }

        // Wander
        if (ImGui::TreeNode("Wander"))
        {
            ImGui::Text("Has Destination: %s", blackboard.hasWanderDestination ? "Yes" : "No");
            ImGui::Text("Destination: (%.1f, %.1f)", 
                blackboard.wanderDestination.x, 
                blackboard.wanderDestination.y);
            ImGui::Text("Wait Timer: %.2f / %.2f s", 
                blackboard.wanderWaitTimer, 
                blackboard.wanderTargetWaitTime);
            ImGui::TreePop();
        }
    }

    void BehaviorTreeDebugWindow::RenderExecutionLog()
    {
        if (ImGui::Button("Clear Log"))
        {
            m_executionLog.clear();
        }

        ImGui::Separator();

        ImGui::BeginChild("ExecutionLogScroll", ImVec2(0, 0), false);

        // Show last 100 entries (newest first)
        for (auto it = m_executionLog.rbegin(); it != m_executionLog.rend(); ++it)
        {
            const auto& entry = *it;

            // Only show entries for selected entity
            if (entry.entity != m_selectedEntity)
                continue;

            // Color based on status
            ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Running (yellow)
            const char* icon = "▶";
            if (entry.status == BTStatus::Success)
            {
                color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Success (green)
                icon = "✓";
            }
            else if (entry.status == BTStatus::Failure)
            {
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Failure (red)
                icon = "✗";
            }

            ImGui::TextColored(color, "[%.2fs ago] %s Node %u (%s)", 
                entry.timeAgo, icon, entry.nodeId, entry.nodeName.c_str());
        }

        ImGui::EndChild();
    }

    void BehaviorTreeDebugWindow::AddExecutionEntry(EntityID entity, uint32_t nodeId, const std::string& nodeName, BTStatus status)
    {
        ExecutionLogEntry entry;
        entry.timeAgo = 0.0f;
        entry.entity = entity;
        entry.nodeId = nodeId;
        entry.nodeName = nodeName;
        entry.status = status;

        m_executionLog.push_back(entry);

        // Keep only last 100 entries
        while (m_executionLog.size() > MAX_LOG_ENTRIES)
        {
            m_executionLog.pop_front();
        }
    }

    void BehaviorTreeDebugWindow::ApplyZoomToStyle()
    {
        ImNodes::GetStyle().NodePadding = ImVec2(8.0f * m_currentZoom, 8.0f * m_currentZoom);
        ImNodes::GetStyle().NodeCornerRounding = 8.0f * m_currentZoom;
        ImNodes::GetStyle().GridSpacing = 32.0f * m_currentZoom;
    }

    void BehaviorTreeDebugWindow::GetGraphBounds(Vector& outMin, Vector& outMax) const
    {
        outMin = Vector(FLT_MAX, FLT_MAX);
        outMax = Vector(-FLT_MAX, -FLT_MAX);
        
        for (const auto& layout : m_currentLayout)
        {
            outMin.x = std::min(outMin.x, layout.position.x - layout.width / 2.0f);
            outMin.y = std::min(outMin.y, layout.position.y - layout.height / 2.0f);
            outMax.x = std::max(outMax.x, layout.position.x + layout.width / 2.0f);
            outMax.y = std::max(outMax.y, layout.position.y + layout.height / 2.0f);
        }
    }

    float BehaviorTreeDebugWindow::GetSafeZoom() const
    {
        // Ensure zoom is always within valid bounds
        return std::max(MIN_ZOOM, std::min(MAX_ZOOM, m_currentZoom));
    }

    Vector BehaviorTreeDebugWindow::CalculatePanOffset(const Vector& graphCenter, const Vector& viewportSize) const
    {
        float safeZoom = GetSafeZoom();
        
        return Vector(
            -graphCenter.x * safeZoom + viewportSize.x / 2.0f,
            -graphCenter.y * safeZoom + viewportSize.y / 2.0f
        );
    }

    void BehaviorTreeDebugWindow::FitGraphToView()
    {
        if (m_currentLayout.empty())
            return;

        // 1. Calculate the bounds of the graph
        Vector minPos, maxPos;
        GetGraphBounds(minPos, maxPos);
        
        Vector graphSize(maxPos.x - minPos.x, maxPos.y - minPos.y);
        ImVec2 imvectmp = ImGui::GetContentRegionAvail();
        Vector viewportSize = Vector(imvectmp.x, imvectmp.y);
        
        // 2. Calculate the zoom needed (protect against division by zero)
        if (graphSize.x <= 0.0f || graphSize.y <= 0.0f)
        {
            // Graph has no dimensions, just center it with current zoom
            CenterViewOnGraph();
            return;
        }
        
        float zoomX = viewportSize.x / graphSize.x;
        float zoomY = viewportSize.y / graphSize.y;
        float targetZoom = std::min(zoomX, zoomY) * 0.9f; // 90% for margins
        
        // 3. Apply the zoom
        m_currentZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, targetZoom));
        ApplyZoomToStyle();
        
        // 4. Center the view
        Vector graphCenter((minPos.x + maxPos.x) / 2.0f, (minPos.y + maxPos.y) / 2.0f);
        Vector panOffset = CalculatePanOffset(graphCenter, viewportSize);
        
        ImNodes::EditorContextResetPanning(ImVec2(panOffset.x, panOffset.y));
        
        std::cout << "[BTDebugger] Fit to view: zoom=" << (int)(m_currentZoom * 100) 
                  << "%, center=(" << (int)graphCenter.x << "," << (int)graphCenter.y << ")" << std::endl;
    }

    void BehaviorTreeDebugWindow::CenterViewOnGraph()
    {
        if (m_currentLayout.empty())
            return;

        Vector minPos, maxPos;
        GetGraphBounds(minPos, maxPos);

        Vector graphCenter((minPos.x + maxPos.x) / 2.0f, (minPos.y + maxPos.y) / 2.0f);
        ImVec2 imvectmp = ImGui::GetContentRegionAvail();
        Vector viewportSize = Vector(imvectmp.x, imvectmp.y);

        Vector panOffset = CalculatePanOffset(graphCenter, viewportSize);

        ImNodes::EditorContextResetPanning(ImVec2(panOffset.x, panOffset.y));
        
        std::cout << "[BTDebugger] Centered view on graph (" << (int)graphCenter.x 
                  << ", " << (int)graphCenter.y << ")" << std::endl;
    }

    void BehaviorTreeDebugWindow::ResetZoom()
    {
        // Recompute layout when resetting zoom
        auto& world = World::Get();
        if (m_selectedEntity != 0 && world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
        {
            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
            
            if (tree)
            {
                m_currentZoom = 1.0f;
                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                ApplyZoomToStyle();
                
                std::cout << "[BTDebugger] Reset zoom to 100% (layout recomputed)" << std::endl;
                return;
            }
        }
        
        // Fallback if no tree
        m_currentZoom = 1.0f;
        ApplyZoomToStyle();
        std::cout << "[BTDebugger] Reset zoom to 100%" << std::endl;
    }

    void BehaviorTreeDebugWindow::RenderMinimap()
    {
        if (m_currentLayout.empty())
            return;

        // Minimap size and position
        const ImVec2 minimapSize(200, 150);
        const ImVec2 minimapPadding(10, 10);
        
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 minimapPos(
            contentMax.x - minimapSize.x - minimapPadding.x,
            contentMax.y - minimapSize.y - minimapPadding.y
        );
        
        ImGui::SetCursorPos(minimapPos);
        
        // Background semi-transparent
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 minimapMin = ImGui::GetCursorScreenPos();
        ImVec2 minimapMax(minimapMin.x + minimapSize.x, minimapMin.y + minimapSize.y);
        
        drawList->AddRectFilled(
            minimapMin, 
            minimapMax, 
            IM_COL32(20, 20, 20, 200), 
            4.0f
        );
        
        // Calculate the bounds of the graph
        Vector graphMin, graphMax;
        GetGraphBounds(graphMin, graphMax);
        
        Vector graphSize(graphMax.x - graphMin.x, graphMax.y - graphMin.y);
        
        // Scale for minimap (protect against division by zero)
        if (graphSize.x <= 0.0f || graphSize.y <= 0.0f)
        {
            // Graph has no dimensions, just show label
            ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
            ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
            return;
        }
        
        float scaleX = minimapSize.x / graphSize.x;
        float scaleY = minimapSize.y / graphSize.y;
        float scale = std::min(scaleX, scaleY) * 0.9f; // Margins
        
        // Additional safety check for scale
        if (scale <= 0.0f)
        {
            ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
            ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
            return;
        }
        
        // Draw nodes
        for (const auto& layout : m_currentLayout)
        {
            float x = minimapMin.x + (layout.position.x - graphMin.x) * scale;
            float y = minimapMin.y + (layout.position.y - graphMin.y) * scale;
            
            ImU32 color = IM_COL32(100, 150, 255, 255); // Blue by default
            
            // Highlight current node
            auto& world = World::Get();
            if (m_selectedEntity != 0 && 
                world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
            {
                const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
                if (layout.nodeId == btRuntime.AICurrentNodeIndex)
                    color = IM_COL32(255, 255, 0, 255); // Yellow
            }
            
            drawList->AddCircleFilled(ImVec2(x, y), 3.0f, color);
        }
        
        // Draw the visible viewport (red rectangle)
        ImVec2 panOffset = ImNodes::EditorContextGetPanning();
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        
        float safeZoom = GetSafeZoom();
        
        float viewMinX = minimapMin.x + (-panOffset.x / safeZoom - graphMin.x) * scale;
        float viewMinY = minimapMin.y + (-panOffset.y / safeZoom - graphMin.y) * scale;
        float viewMaxX = viewMinX + (viewportSize.x / safeZoom) * scale;
        float viewMaxY = viewMinY + (viewportSize.y / safeZoom) * scale;
        
        drawList->AddRect(
            ImVec2(viewMinX, viewMinY),
            ImVec2(viewMaxX, viewMaxY),
            IM_COL32(255, 0, 0, 255),
            0.0f,
            0,
            2.0f
        );
        
        // Interaction: click to center
        ImGui::SetCursorPos(minimapPos);
        ImGui::InvisibleButton("##minimap", minimapSize);
        
        if (ImGui::IsItemClicked())
        {
            ImVec2 clickPos = ImGui::GetMousePos();
            float clickX = (clickPos.x - minimapMin.x) / scale + graphMin.x;
            float clickY = (clickPos.y - minimapMin.y) / scale + graphMin.y;
            Vector clickPosVec(clickX, clickY);
            Vector newPan = CalculatePanOffset(clickPosVec, Vector(viewportSize.x, viewportSize.y));
            ImNodes::EditorContextResetPanning(ImVec2(newPan.x, newPan.y));
        }
        
        // Label
        ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
    }
    
    // ========================================================================
    // Editor Mode Functions
    // ========================================================================
    
    void BehaviorTreeDebugWindow::RenderEditorToolbar()
    {
        if (ImGui::Button("Add Node"))
        {
            m_showNodePalette = true;
            m_nodeCreationPos = ImGui::GetMousePos();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Save Tree"))
        {
            SaveEditedTree();
        }
        
        ImGui::SameLine();
        bool canUndo = !m_undoStack.empty();
        if (!canUndo) ImGui::BeginDisabled();
        if (ImGui::Button("Undo"))
        {
            UndoLastAction();
        }
        if (!canUndo) ImGui::EndDisabled();
        
        ImGui::SameLine();
        bool canRedo = !m_redoStack.empty();
        if (!canRedo) ImGui::BeginDisabled();
        if (ImGui::Button("Redo"))
        {
            RedoLastAction();
        }
        if (!canRedo) ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::Text("Selected: %zu", m_selectedNodes.size());
    }
    
    void BehaviorTreeDebugWindow::RenderNodePalette()
    {
        ImGui::OpenPopup("##NodePalette");
        
        if (ImGui::BeginPopup("##NodePalette"))
        {
            ImGui::Text("Add Node");
            ImGui::Separator();
            
            if (ImGui::MenuItem("Selector"))
            {
                HandleNodeCreation(BTNodeType::Selector);
                m_showNodePalette = false;
            }
            
            if (ImGui::MenuItem("Sequence"))
            {
                HandleNodeCreation(BTNodeType::Sequence);
                m_showNodePalette = false;
            }
            
            if (ImGui::MenuItem("Condition"))
            {
                HandleNodeCreation(BTNodeType::Condition);
                m_showNodePalette = false;
            }
            
            if (ImGui::MenuItem("Action"))
            {
                HandleNodeCreation(BTNodeType::Action);
                m_showNodePalette = false;
            }
            
            if (ImGui::MenuItem("Inverter"))
            {
                HandleNodeCreation(BTNodeType::Inverter);
                m_showNodePalette = false;
            }
            
            if (ImGui::MenuItem("Repeater"))
            {
                HandleNodeCreation(BTNodeType::Repeater);
                m_showNodePalette = false;
            }
            
            ImGui::EndPopup();
        }
        else
        {
            m_showNodePalette = false;
        }
    }
    
    void BehaviorTreeDebugWindow::HandleNodeCreation(BTNodeType nodeType)
    {
        // Create new node
        BTNode newNode;
        newNode.type = nodeType;
        newNode.id = m_nextNodeId++;
        
        // Set default name based on type
        switch (nodeType)
        {
            case BTNodeType::Selector:
                newNode.name = "New Selector";
                break;
            case BTNodeType::Sequence:
                newNode.name = "New Sequence";
                break;
            case BTNodeType::Condition:
                newNode.name = "New Condition";
                newNode.conditionType = BTConditionType::TargetVisible;
                break;
            case BTNodeType::Action:
                newNode.name = "New Action";
                newNode.actionType = BTActionType::Idle;
                break;
            case BTNodeType::Inverter:
                newNode.name = "New Inverter";
                break;
            case BTNodeType::Repeater:
                newNode.name = "New Repeater";
                newNode.repeatCount = 1;
                break;
        }
        
        // Initialize editing tree if needed
        if (m_editingTree.nodes.empty() && m_selectedEntity != 0)
        {
            auto& world = World::Get();
            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
            const BehaviorTreeAsset* originalTree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
            
            if (originalTree)
            {
                m_editingTree = *originalTree;
            }
            else
            {
                // Create new empty tree
                m_editingTree.id = btRuntime.AITreeAssetId;
                m_editingTree.name = "New Tree";
                m_editingTree.rootNodeId = 0;
            }
        }
        
        // Add to editing tree
        m_editingTree.nodes.push_back(newNode);
        
        // Add to undo stack
        EditorAction action;
        action.type = EditorAction::AddNode;
        action.nodeData = newNode;
        m_undoStack.push_back(action);
        if (m_undoStack.size() > kMaxUndoStackSize)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
        m_redoStack.clear();
        
        m_treeModified = true;
        
        // Recompute layout
        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
        
        std::cout << "[BTEditor] Created node: " << newNode.name << " (ID: " << newNode.id << ")" << std::endl;
    }
    
    void BehaviorTreeDebugWindow::HandleNodeDeletion()
    {
        if (m_selectedNodes.empty())
            return;
        
        // Delete each selected node
        for (uint32_t nodeId : m_selectedNodes)
        {
            // Find node in editing tree
            auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                [nodeId](const BTNode& n) { return n.id == nodeId; });
            
            if (it != m_editingTree.nodes.end())
            {
                // Add to undo stack
                EditorAction action;
                action.type = EditorAction::DeleteNode;
                action.nodeData = *it;
                m_undoStack.push_back(action);
                if (m_undoStack.size() > kMaxUndoStackSize)
                {
                    m_undoStack.erase(m_undoStack.begin());
                }
                
                // Remove from tree
                m_editingTree.nodes.erase(it);
                
                // Remove any connections to/from this node
                for (auto& node : m_editingTree.nodes)
                {
                    // Remove from childIds
                    auto childIt = std::find(node.childIds.begin(), node.childIds.end(), nodeId);
                    if (childIt != node.childIds.end())
                    {
                        node.childIds.erase(childIt);
                    }
                    
                    // Remove from decoratorChildId
                    if (node.decoratorChildId == nodeId)
                    {
                        node.decoratorChildId = 0;
                    }
                }
                
                std::cout << "[BTEditor] Deleted node ID: " << nodeId << std::endl;
            }
        }
        
        m_selectedNodes.clear();
        m_redoStack.clear();
        m_treeModified = true;
        
        // Recompute layout
        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
    }
    
    void BehaviorTreeDebugWindow::HandleNodeDuplication()
    {
        if (m_selectedNodes.empty())
            return;
        
        std::vector<uint32_t> newNodes;
        
        // Duplicate each selected node
        for (uint32_t nodeId : m_selectedNodes)
        {
            // Find node in editing tree
            auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                [nodeId](const BTNode& n) { return n.id == nodeId; });
            
            if (it != m_editingTree.nodes.end())
            {
                // Create duplicate
                BTNode duplicate = *it;
                duplicate.id = m_nextNodeId++;
                duplicate.name = duplicate.name + " (Copy)";
                
                // Add to tree
                m_editingTree.nodes.push_back(duplicate);
                newNodes.push_back(duplicate.id);
                
                // Add to undo stack
                EditorAction action;
                action.type = EditorAction::AddNode;
                action.nodeData = duplicate;
                m_undoStack.push_back(action);
                
                std::cout << "[BTEditor] Duplicated node: " << duplicate.name << " (ID: " << duplicate.id << ")" << std::endl;
            }
        }
        
        // Select the new nodes
        m_selectedNodes = newNodes;
        m_redoStack.clear();
        m_treeModified = true;
        
        // Recompute layout
        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
    }
    
    bool BehaviorTreeDebugWindow::ValidateConnection(uint32_t parentId, uint32_t childId) const
    {
        // Find parent and child nodes
        const BTNode* parent = m_editingTree.GetNode(parentId);
        const BTNode* child = m_editingTree.GetNode(childId);
        
        if (!parent || !child)
            return false;
        
        // Prevent self-connection
        if (parentId == childId)
            return false;
        
        // Only composite and decorator nodes can have children
        if (parent->type != BTNodeType::Selector &&
            parent->type != BTNodeType::Sequence &&
            parent->type != BTNodeType::Inverter &&
            parent->type != BTNodeType::Repeater)
        {
            return false;
        }
        
        // Decorator nodes (Inverter, Repeater) can only have one child
        if ((parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater) &&
            parent->decoratorChildId != 0)
        {
            return false;
        }
        
        // Prevent duplicate connections
        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
        {
            if (std::find(parent->childIds.begin(), parent->childIds.end(), childId) != parent->childIds.end())
            {
                return false;
            }
        }
        
        // Prevent cycles: Check if parent is a descendant of child
        // Use DFS to traverse from child to see if we can reach parent
        std::vector<uint32_t> visited;
        std::vector<uint32_t> toVisit;
        toVisit.push_back(childId);
        
        while (!toVisit.empty())
        {
            uint32_t currentId = toVisit.back();
            toVisit.pop_back();
            
            if (currentId == parentId)
            {
                // Found parent as descendant of child - would create cycle
                return false;
            }
            
            if (std::find(visited.begin(), visited.end(), currentId) != visited.end())
            {
                continue;  // Already visited
            }
            
            visited.push_back(currentId);
            
            const BTNode* current = m_editingTree.GetNode(currentId);
            if (current)
            {
                // Add children to visit list
                for (uint32_t id : current->childIds)
                {
                    toVisit.push_back(id);
                }
                if (current->decoratorChildId != 0)
                {
                    toVisit.push_back(current->decoratorChildId);
                }
            }
        }
        
        return true;
    }
    
    void BehaviorTreeDebugWindow::SaveEditedTree()
    {
        if (!m_treeModified)
        {
            std::cout << "[BTEditor] No changes to save" << std::endl;
            return;
        }
        
        // Generate JSON for the edited tree
        json treeJson;
        treeJson["schema_version"] = 2;
        treeJson["type"] = "BehaviorTree";
        treeJson["blueprintType"] = "BehaviorTree";
        treeJson["name"] = m_editingTree.name;
        treeJson["description"] = "Edited in BT Editor";
        
        // Metadata
        json metadata;
        metadata["author"] = "BT Editor";
        // Get current timestamp
        auto now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));
        metadata["created"] = timestamp;
        metadata["lastModified"] = timestamp;
        metadata["tags"] = json::array({"AI", "BehaviorTree", "Edited"});
        treeJson["metadata"] = metadata;
        
        // Editor state
        json editorState;
        editorState["zoom"] = 1.0;
        editorState["scrollOffset"] = {{"x", 0}, {"y", 0}};
        treeJson["editorState"] = editorState;
        
        // Data section
        json dataSection;
        dataSection["rootNodeId"] = m_editingTree.rootNodeId;
        
        // Nodes array
        json nodesArray = json::array();
        for (const auto& node : m_editingTree.nodes)
        {
            json nodeJson;
            nodeJson["id"] = node.id;
            nodeJson["name"] = node.name;
            
            // Node type
            switch (node.type)
            {
                case BTNodeType::Selector: nodeJson["type"] = "Selector"; break;
                case BTNodeType::Sequence: nodeJson["type"] = "Sequence"; break;
                case BTNodeType::Condition: nodeJson["type"] = "Condition"; break;
                case BTNodeType::Action: nodeJson["type"] = "Action"; break;
                case BTNodeType::Inverter: nodeJson["type"] = "Inverter"; break;
                case BTNodeType::Repeater: nodeJson["type"] = "Repeater"; break;
            }
            
            // Position (default if not available)
            nodeJson["position"] = {{"x", 0.0}, {"y", 0.0}};
            
            // Node-specific data
            if (node.type == BTNodeType::Condition)
            {
                // Map enum to string
                const char* conditionTypeStr = "TargetVisible";
                switch (node.conditionType)
                {
                    case BTConditionType::TargetVisible: conditionTypeStr = "TargetVisible"; break;
                    case BTConditionType::TargetInRange: conditionTypeStr = "TargetInRange"; break;
                    case BTConditionType::HealthBelow: conditionTypeStr = "HealthBelow"; break;
                    case BTConditionType::HasMoveGoal: conditionTypeStr = "HasMoveGoal"; break;
                    case BTConditionType::CanAttack: conditionTypeStr = "CanAttack"; break;
                    case BTConditionType::HeardNoise: conditionTypeStr = "HeardNoise"; break;
                    case BTConditionType::IsWaitTimerExpired: conditionTypeStr = "IsWaitTimerExpired"; break;
                    case BTConditionType::HasNavigableDestination: conditionTypeStr = "HasNavigableDestination"; break;
                    case BTConditionType::HasValidPath: conditionTypeStr = "HasValidPath"; break;
                    case BTConditionType::HasReachedDestination: conditionTypeStr = "HasReachedDestination"; break;
                }
                nodeJson["conditionType"] = conditionTypeStr;
                if (node.conditionParam != 0.0f)
                {
                    nodeJson["parameters"] = {{"param", node.conditionParam}};
                }
                else
                {
                    nodeJson["parameters"] = json::object();
                }
            }
            else if (node.type == BTNodeType::Action)
            {
                // Map enum to string
                const char* actionTypeStr = "Idle";
                switch (node.actionType)
                {
                    case BTActionType::SetMoveGoalToLastKnownTargetPos: actionTypeStr = "SetMoveGoalToLastKnownTargetPos"; break;
                    case BTActionType::SetMoveGoalToTarget: actionTypeStr = "SetMoveGoalToTarget"; break;
                    case BTActionType::SetMoveGoalToPatrolPoint: actionTypeStr = "SetMoveGoalToPatrolPoint"; break;
                    case BTActionType::MoveToGoal: actionTypeStr = "MoveToGoal"; break;
                    case BTActionType::AttackIfClose: actionTypeStr = "AttackIfClose"; break;
                    case BTActionType::PatrolPickNextPoint: actionTypeStr = "PatrolPickNextPoint"; break;
                    case BTActionType::ClearTarget: actionTypeStr = "ClearTarget"; break;
                    case BTActionType::Idle: actionTypeStr = "Idle"; break;
                    case BTActionType::WaitRandomTime: actionTypeStr = "WaitRandomTime"; break;
                    case BTActionType::ChooseRandomNavigablePoint: actionTypeStr = "ChooseRandomNavigablePoint"; break;
                    case BTActionType::RequestPathfinding: actionTypeStr = "RequestPathfinding"; break;
                    case BTActionType::FollowPath: actionTypeStr = "FollowPath"; break;
                }
                nodeJson["actionType"] = actionTypeStr;
                json params;
                if (node.actionParam1 != 0.0f) params["param1"] = node.actionParam1;
                if (node.actionParam2 != 0.0f) params["param2"] = node.actionParam2;
                nodeJson["parameters"] = params.empty() ? json::object() : params;
            }
            else if (node.type == BTNodeType::Repeater)
            {
                nodeJson["repeatCount"] = node.repeatCount;
            }
            
            // Children
            if (!node.childIds.empty())
            {
                nodeJson["childIds"] = node.childIds;
            }
            if (node.decoratorChildId != 0)
            {
                nodeJson["decoratorChildId"] = node.decoratorChildId;
            }
            
            nodesArray.push_back(nodeJson);
        }
        
        dataSection["nodes"] = nodesArray;
        treeJson["data"] = dataSection;
        
        // Save to file
        std::string filename = "Blueprints/AI/" + m_editingTree.name + "_edited.json";
        
        try
        {
            std::ofstream file(filename);
            if (file.is_open())
            {
                file << treeJson.dump(2);  // Pretty print with 2-space indentation
                file.close();
                
                m_treeModified = false;
                std::cout << "[BTEditor] Tree saved to: " << filename << std::endl;
            }
            else
            {
                std::cerr << "[BTEditor] ERROR: Failed to open file for writing: " << filename << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[BTEditor] ERROR: Exception during save: " << e.what() << std::endl;
        }
    }
    
    void BehaviorTreeDebugWindow::UndoLastAction()
    {
        if (m_undoStack.empty())
            return;
        
        EditorAction action = m_undoStack.back();
        m_undoStack.pop_back();
        
        // Perform undo
        switch (action.type)
        {
            case EditorAction::AddNode:
            {
                // Remove the added node
                auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                    [&action](const BTNode& n) { return n.id == action.nodeData.id; });
                if (it != m_editingTree.nodes.end())
                {
                    m_editingTree.nodes.erase(it);
                }
                break;
            }
            case EditorAction::DeleteNode:
            {
                // Restore the deleted node
                m_editingTree.nodes.push_back(action.nodeData);
                break;
            }
            case EditorAction::AddConnection:
            {
                // Remove the connection
                BTNode* parent = m_editingTree.GetNode(action.parentId);
                if (parent)
                {
                    if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                    {
                        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), action.childId);
                        if (it != parent->childIds.end())
                        {
                            parent->childIds.erase(it);
                        }
                    }
                    else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                    {
                        parent->decoratorChildId = 0;
                    }
                }
                break;
            }
            case EditorAction::DeleteConnection:
            {
                // Restore the connection
                BTNode* parent = m_editingTree.GetNode(action.parentId);
                if (parent)
                {
                    if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                    {
                        parent->childIds.push_back(action.childId);
                    }
                    else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                    {
                        parent->decoratorChildId = action.childId;
                    }
                }
                break;
            }
            default:
                break;
        }
        
        // Add to redo stack
        m_redoStack.push_back(action);
        
        // Recompute layout
        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
        
        std::cout << "[BTEditor] Undo performed" << std::endl;
    }
    
    void BehaviorTreeDebugWindow::RedoLastAction()
    {
        if (m_redoStack.empty())
            return;
        
        EditorAction action = m_redoStack.back();
        m_redoStack.pop_back();
        
        // Perform redo
        switch (action.type)
        {
            case EditorAction::AddNode:
            {
                // Re-add the node
                m_editingTree.nodes.push_back(action.nodeData);
                break;
            }
            case EditorAction::DeleteNode:
            {
                // Re-delete the node
                auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                    [&action](const BTNode& n) { return n.id == action.nodeData.id; });
                if (it != m_editingTree.nodes.end())
                {
                    m_editingTree.nodes.erase(it);
                }
                break;
            }
            case EditorAction::AddConnection:
            {
                // Re-add the connection
                BTNode* parent = m_editingTree.GetNode(action.parentId);
                if (parent)
                {
                    if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                    {
                        parent->childIds.push_back(action.childId);
                    }
                    else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                    {
                        parent->decoratorChildId = action.childId;
                    }
                }
                break;
            }
            case EditorAction::DeleteConnection:
            {
                // Re-delete the connection
                BTNode* parent = m_editingTree.GetNode(action.parentId);
                if (parent)
                {
                    if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                    {
                        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), action.childId);
                        if (it != parent->childIds.end())
                        {
                            parent->childIds.erase(it);
                        }
                    }
                    else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
                    {
                        parent->decoratorChildId = 0;
                    }
                }
                break;
            }
            default:
                break;
        }
        
        // Add to undo stack
        m_undoStack.push_back(action);
        if (m_undoStack.size() > kMaxUndoStackSize)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
        
        // Recompute layout
        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
        
        std::cout << "[BTEditor] Redo performed" << std::endl;
    }
}
