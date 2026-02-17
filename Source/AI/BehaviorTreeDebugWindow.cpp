/**
 * @file BehaviorTreeDebugWindow.cpp
 * @brief Implementation of behavior tree runtime debugger
 */

#include "BehaviorTreeDebugWindow.h"
#include "BTEditorCommand.h"
#include "../World.h"
#include "../GameEngine.h"
#include "../ECS_Components.h"
#include "../ECS_Components_AI.h"
#include "../json_helper.h"
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
#include <set>

using json = nlohmann::json;

namespace Olympe
{
    // Camera zoom constants
    constexpr float MIN_ZOOM = 0.3f;
    constexpr float MAX_ZOOM = 3.0f;
    constexpr float ZOOM_EPSILON = 0.001f;

    BehaviorTreeDebugWindow::BehaviorTreeDebugWindow()
        : m_separateWindow(nullptr)
        , m_separateRenderer(nullptr)
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

        LoadBTConfig();

        if (!m_imnodesInitialized)
        {
            ImNodes::CreateContext();
            ImNodes::GetStyle().GridSpacing = 32.0f;
            ImNodes::GetStyle().NodeCornerRounding = 8.0f;
            ImNodes::GetStyle().NodePadding = ImVec2(8, 8);
            m_imnodesInitialized = true;
        }

        ApplyConfigToLayout();

        m_isInitialized = true;

        std::cout << "[BTDebugger] Initialized (window will be created on first F10)" << std::endl;
    }

    void BehaviorTreeDebugWindow::Shutdown()
    {
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

        ImGuiContext* previousContext = ImGui::GetCurrentContext();

        const int windowWidth = 1200;
        const int windowHeight = 720;

        if (!SDL_CreateWindowAndRenderer(
            "Behavior Tree Runtime Debugger - Independent Window",
            windowWidth,
            windowHeight,
            SDL_WINDOW_RESIZABLE,
            &m_separateWindow,
            &m_separateRenderer))
        {
            std::cout << "[BTDebugger] ERROR: Failed to create separate window: "
                << SDL_GetError() << std::endl;
            return;
        }

        m_separateImGuiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_separateImGuiContext);

        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForSDLRenderer(m_separateWindow, m_separateRenderer);
        ImGui_ImplSDLRenderer3_Init(m_separateRenderer);

        m_windowCreated = true;

        ImGui::SetCurrentContext(previousContext);

        std::cout << "[BTDebugger] ✅ Separate window created successfully!" << std::endl;
        std::cout << "[BTDebugger] Window can be moved to second monitor" << std::endl;
    }

    void BehaviorTreeDebugWindow::DestroySeparateWindow()
    {
        if (!m_windowCreated)
            return;

        ImGuiContext* previousContext = ImGui::GetCurrentContext();

        if (m_separateImGuiContext != nullptr)
        {
            ImGui::SetCurrentContext(m_separateImGuiContext);
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(m_separateImGuiContext);
            m_separateImGuiContext = nullptr;
        }

        if (previousContext != m_separateImGuiContext)
            ImGui::SetCurrentContext(previousContext);

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

        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            if (event->window.windowID == SDL_GetWindowID(m_separateWindow))
            {
                ToggleVisibility();
                return;
            }
        }

        ImGuiContext* previousContext = ImGui::GetCurrentContext();

        ImGui::SetCurrentContext(m_separateImGuiContext);
        ImGui_ImplSDL3_ProcessEvent(event);

        ImGui::SetCurrentContext(previousContext);
    }

    void BehaviorTreeDebugWindow::Render()
    {
        if (!m_isVisible || !m_windowCreated)
            return;

        ImGuiContext* previousContext = ImGui::GetCurrentContext();

        ImGui::SetCurrentContext(m_separateImGuiContext);

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        RenderInSeparateWindow();

        ImGui::Render();
        SDL_SetRenderDrawColor(m_separateRenderer, 18, 18, 20, 255);
        SDL_RenderClear(m_separateRenderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_separateRenderer);
        SDL_RenderPresent(m_separateRenderer);

        ImGui::SetCurrentContext(previousContext);
    }

    void BehaviorTreeDebugWindow::RenderInSeparateWindow()
    {
        m_pulseTimer += GameEngine::fDt;

        static float accumulatedTime = 0.0f;
        accumulatedTime += GameEngine::fDt;

        if (accumulatedTime >= m_autoRefreshInterval)
        {
            RefreshEntityList();
            accumulatedTime = 0.0f;
        }

        for (auto& entry : m_executionLog)
        {
            entry.timeAgo += GameEngine::fDt;
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (!ImGui::Begin("Behavior Tree Runtime Debugger##Main", nullptr, windowFlags))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                ImGui::SliderFloat("Auto Refresh (s)", &m_autoRefreshInterval, 0.1f, 5.0f);
                ImGui::SliderFloat("Entity List Width", &m_entityListWidth, 150.0f, 400.0f);
                ImGui::SliderFloat("Inspector Width", &m_inspectorWidth, 250.0f, 500.0f);

                if (ImGui::SliderFloat("Node Spacing X", &m_nodeSpacingX, 80.0f, 400.0f))
                {
                    m_needsLayoutUpdate = true;
                }
                if (ImGui::SliderFloat("Node Spacing Y", &m_nodeSpacingY, 60.0f, 300.0f))
                {
                    m_needsLayoutUpdate = true;
                }

                if (ImGui::Button("Reset Spacing to Defaults"))
                {
                    m_nodeSpacingX = 180.0f;
                    m_nodeSpacingY = 120.0f;
                    m_needsLayoutUpdate = true;
                }

                ImGui::Separator();

                if (ImGui::Checkbox("Grid Snapping", &m_config.gridSnappingEnabled))
                {
                    std::cout << "[BTDebugger] Grid snapping "
                        << (m_config.gridSnappingEnabled ? "enabled" : "disabled") << std::endl;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Snap node positions to %.0fpx grid", m_config.gridSize);
                }

                ImGui::Separator();
                ImGui::Text("Current Zoom: %.0f%%", m_currentZoom * 100.0f);
                ImGui::Checkbox("Show Minimap", &m_showMinimap);

                ImGui::Checkbox("Auto-Fit on Load", &m_autoFitOnLoad);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Automatically fit tree to view when selecting an entity");
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Reload Config"))
                {
                    LoadBTConfig();
                    ApplyConfigToLayout();
                    m_needsLayoutUpdate = true;
                    std::cout << "[BTDebugger] Configuration reloaded" << std::endl;
                }

                ImGui::Separator();
                ImGui::Text("Window Mode: Separate (Independent)");
                ImGui::Text("Press F10 to close window");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Auto Organize Graph"))
                {
                    if (m_selectedEntity != 0)
                    {
                        auto& world = World::Get();
                        if (world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
                        {
                            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
                            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
                            if (tree)
                            {
                                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                                std::cout << "[BTDebugger] Graph reorganized with current settings" << std::endl;
                            }
                        }
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Recalculate all node positions using the current layout algorithm");
                }

                if (ImGui::MenuItem("Reset View"))
                {
                    ResetZoom();
                    CenterViewOnGraph();
                    std::cout << "[BTDebugger] View reset to default (zoom 100%%, centered)" << std::endl;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Reset zoom to 100%% and center camera");
                }

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

        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            RefreshEntityList();
        }

        float windowWidth = ImGui::GetContentRegionAvail().x;
        float windowHeight = ImGui::GetContentRegionAvail().y;

        ImGui::BeginChild("EntityListPanel", ImVec2(m_entityListWidth, windowHeight), true);
        RenderEntityListPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        float centerWidth = windowWidth - m_entityListWidth - m_inspectorWidth - 20.0f;
        ImGui::BeginChild("NodeGraphPanel", ImVec2(centerWidth, windowHeight), true);
        RenderNodeGraphPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("InspectorPanel", ImVec2(m_inspectorWidth, windowHeight), true);
        RenderInspectorPanel();
        ImGui::EndChild();

        ImGui::End();
    }

    void BehaviorTreeDebugWindow::RefreshEntityList()
    {
        m_entities.clear();

        auto& world = World::Get();
        const auto& allEntities = world.GetAllEntities();

        for (EntityID entity : allEntities)
        {
            if (!world.HasComponent<BehaviorTreeRuntime_data>(entity))
                continue;

            EntityDebugInfo info;
            info.entityId = entity;

            if (world.HasComponent<Identity_data>(entity))
            {
                const auto& identity = world.GetComponent<Identity_data>(entity);
                info.entityName = identity.name;
            }
            else
            {
                info.entityName = "Entity " + std::to_string(entity);
            }

            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(entity);
            info.treeId = btRuntime.AITreeAssetId;
            info.isActive = btRuntime.isActive;
            info.currentNodeId = btRuntime.AICurrentNodeIndex;
            info.lastStatus = static_cast<BTStatus>(btRuntime.lastStatus);

            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
            if (tree)
            {
                info.treeName = tree->name;
            }
            else
            {
                std::string path = BehaviorTreeManager::Get().GetTreePathFromId(info.treeId);

                if (!path.empty())
                {
                    info.treeName = "Not Loaded: " + path;
                }
                else
                {
                    info.treeName = "Unknown (ID=" + std::to_string(info.treeId) + ")";
                }

                static std::set<EntityID> debuggedEntities;
                if (debuggedEntities.find(entity) == debuggedEntities.end())
                {
                    debuggedEntities.insert(entity);
                    std::cout << "[BTDebugger] WARNING: Entity " << entity << " (" << info.entityName
                        << ") has unknown tree ID=" << info.treeId << std::endl;
                    BehaviorTreeManager::Get().DebugPrintLoadedTrees();
                }
            }

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

            if (world.HasComponent<AIBlackboard_data>(entity))
            {
                const auto& blackboard = world.GetComponent<AIBlackboard_data>(entity);
                info.hasTarget = blackboard.hasTarget;
            }

            info.lastUpdateTime = 0.0f;

            m_entities.push_back(info);
        }

        UpdateEntityFiltering();
        UpdateEntitySorting();
    }

    void BehaviorTreeDebugWindow::UpdateEntityFiltering()
    {
        m_filteredEntities.clear();

        for (const auto& info : m_entities)
        {
            if (m_filterText[0] != '\0')
            {
                std::string lowerName = info.entityName;
                std::string lowerFilter = m_filterText;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

                if (lowerName.find(lowerFilter) == std::string::npos)
                    continue;
            }

            if (m_filterActiveOnly && !info.isActive)
                continue;

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
        ImGui::PushID((unsigned int)info.entityId);

        const char* statusIcon = info.isActive ? "●" : "○";
        ImVec4 statusColor = info.isActive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        const char* resultIcon = "▶";
        ImVec4 resultColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
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

        bool isSelected = (m_selectedEntity == info.entityId);
        ImGui::TextColored(statusColor, "%s", statusIcon);
        ImGui::SameLine();
        ImGui::TextColored(resultColor, "%s", resultIcon);
        ImGui::SameLine();

        if (ImGui::Selectable(info.entityName.c_str(), isSelected))
        {
            m_selectedEntity = info.entityId;

            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
            if (tree)
            {
                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                m_needsLayoutUpdate = false;

                if (m_autoFitOnLoad)
                {
                    // Auto-fit logic will trigger in graph panel
                }
            }
        }

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

        auto& world = World::Get();
        if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
        {
            ImGui::Text("Selected entity no longer has a behavior tree");
            m_selectedEntity = 0;
            return;
        }

        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);

        const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

        if (!tree)
        {
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

        bool prevEditorMode = m_editorMode;
        ImGui::Checkbox("Editor Mode", &m_editorMode);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Enable editing mode to add/remove/connect nodes");
        }

        if (m_editorMode && !prevEditorMode)
        {
            const BehaviorTreeAsset* originalTree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

            if (originalTree)
            {
                m_editingTree = *originalTree;
                m_treeModified = false;
                m_selectedNodes.clear();
                m_undoStack.clear();
                m_redoStack.clear();
                m_nextNodeId = 1000;
                m_nextLinkId = 100000;

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

            RenderEditorToolbar();
        }

        ImGui::Separator();

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

        if (layoutChanged)
        {
            m_layoutEngine.SetLayoutDirection(m_layoutDirection);
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            m_needsLayoutUpdate = false;
        }

        if (m_needsLayoutUpdate && tree)
        {
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            m_needsLayoutUpdate = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset Camera"))
        {
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

        ImNodes::BeginNodeEditor();

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

            if (m_lastCenteredEntity != m_selectedEntity)
            {
                if (m_editorMode)
                {
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
                    FitGraphToView();
                }
                else
                {
                    CenterViewOnGraph();
                }

                std::cout << "[BTDebugger] ✅ Camera " << (m_autoFitOnLoad ? "fitted" : "centered") << " on graph" << std::endl;
                m_lastCenteredEntity = m_selectedEntity;
            }
        }

        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            if (io.MouseWheel != 0.0f)
            {
                float oldZoom = m_currentZoom;
                float zoomDelta = io.MouseWheel * 0.1f;
                m_currentZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, m_currentZoom + zoomDelta));

                if (std::abs(m_currentZoom - oldZoom) > ZOOM_EPSILON && tree)
                {
                    m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                    ApplyZoomToStyle();

                    std::cout << "[BTDebugger] Zoom: " << (int)(m_currentZoom * 100)
                        << "% (layout recomputed)" << std::endl;
                }
            }
        }

        if (ImGui::IsWindowFocused())
        {
            bool ctrlPressed = io.KeyCtrl;

            if (ImGui::IsKeyPressed(ImGuiKey_F) && !ctrlPressed)
                FitGraphToView();

            if (ImGui::IsKeyPressed(ImGuiKey_C) && !ctrlPressed)
                CenterViewOnGraph();

            if ((ImGui::IsKeyPressed(ImGuiKey_0) || ImGui::IsKeyPressed(ImGuiKey_Keypad0)) && !ctrlPressed)
                ResetZoom();

            if (ImGui::IsKeyPressed(ImGuiKey_M) && !ctrlPressed)
                m_showMinimap = !m_showMinimap;

            if ((ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) && !ctrlPressed)
            {
                float oldZoom = m_currentZoom;
                m_currentZoom = std::min(MAX_ZOOM, m_currentZoom * 1.2f);

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

                if (std::abs(m_currentZoom - oldZoom) > ZOOM_EPSILON && tree)
                {
                    m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
                    ApplyZoomToStyle();
                }
            }
        }

        RenderBehaviorTreeGraph();

        if (m_showMinimap)
            RenderMinimap();

        ImNodes::EndNodeEditor();

        if (m_editorMode)
        {
            int startAttrId, endAttrId;
            if (ImNodes::IsLinkCreated(&startAttrId, &endAttrId))
            {
                uint32_t parentId = startAttrId / 10000;
                uint32_t childId = endAttrId / 10000;

                if (ValidateConnection(parentId, childId))
                {
                    BTNode* parent = m_editingTree.GetNode(parentId);
                    if (parent)
                    {
                        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                        {
                            int childIndex = (int) parent->childIds.size();
                            parent->childIds.push_back(childId);

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
                            parent->decoratorChildId = childId;

                            EditorAction action;
                            action.type = EditorAction::AddConnection;
                            action.parentId = parentId;
                            action.childId = childId;
                            action.childIndex = 0;
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

            int linkId;
            if (ImNodes::IsLinkDestroyed(&linkId))
            {
                auto it = std::find_if(m_linkMap.begin(), m_linkMap.end(),
                    [linkId](const LinkInfo& info) { return info.linkId == linkId; });

                if (it != m_linkMap.end())
                {
                    uint32_t parentId = it->parentId;
                    uint32_t childId = it->childId;

                    BTNode* parent = m_editingTree.GetNode(parentId);
                    if (parent)
                    {
                        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
                        {
                            auto childIt = std::find(parent->childIds.begin(), parent->childIds.end(), childId);
                            if (childIt != parent->childIds.end())
                            {
                                int childIndex = (int) std::distance(parent->childIds.begin(), childIt);
                                parent->childIds.erase(childIt);

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

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                m_showNodePalette = true;
                m_nodeCreationPos.Set(ImGui::GetMousePos().x, ImGui::GetMousePos().y, 0.f);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_selectedNodes.empty())
            {
                HandleNodeDeletion();
            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && !m_selectedNodes.empty())
            {
                HandleNodeDuplication();
            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
            {
                UndoLastAction();
            }

            if (ImGui::GetIO().KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) ||
                (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
            {
                RedoLastAction();
            }
        }

		// ImNodes::EndNodeEditor(); // remove second call to EndNodeEditor TO BE REMOVED it causes crashes when interacting with the graph, but is needed to render the minimap correctly. Need to refactor rendering flow to avoid this hack

        if (m_showNodePalette)
        {
            RenderNodePalette();
        }
    }

    void BehaviorTreeDebugWindow::RenderBehaviorTreeGraph()
    {
        auto& world = World::Get();
        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);

        const BehaviorTreeAsset* tree = nullptr;

        if (m_editorMode && !m_editingTree.nodes.empty())
        {
            tree = &m_editingTree;
        }
        else
        {
            tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
        }

        if (!tree)
            return;

        uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;

        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                bool isCurrentNode = (node.id == currentNodeId) && btRuntime.isActive && !m_editorMode;
                RenderNode(&node, layout, isCurrentNode);
            }
        }

        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                RenderNodeConnections(&node, layout, tree);
            }
        }

        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                RenderNodePins(&node, layout);
            }
        }
    }

    void BehaviorTreeDebugWindow::RenderNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode)
    {
        static std::unordered_set<uint32_t> printedNodeIds;

        if (!node || !layout)
            return;

        if (printedNodeIds.find(node->id) == printedNodeIds.end())
        {
            std::cout << "[RenderNode] Node " << node->id
                << " (" << node->name << ") at ("
                << (int)layout->position.x << ", " << (int)layout->position.y << ")" << std::endl;
            printedNodeIds.insert(node->id);
        }

        ImNodes::SetNodeGridSpacePos(node->id, ImVec2(layout->position.x, layout->position.y));

        ImNodes::BeginNode(node->id);

        ImNodes::BeginNodeTitleBar();

        BTStatus nodeStatus = isCurrentNode ? BTStatus::Running : BTStatus::Idle;

        uint32_t color = GetNodeColorByStatus(node->type, nodeStatus);
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, color);
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, color);
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, color);

        const char* icon = GetNodeIcon(node->type);
        ImGui::Text("%s %s", icon, node->name.c_str());

        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();

        ImNodes::EndNodeTitleBar();

        ImGui::PushItemWidth(200);

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

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ID: %u", node->id);

        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::PopItemWidth();

        if (node->id != 0)
        {
            ImNodes::BeginInputAttribute(node->id * 10000);
            ImGui::Text("In");
            ImNodes::EndInputAttribute();
        }

        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence ||
            node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater)
        {
            ImNodes::BeginOutputAttribute(node->id * 10000 + 1);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();
        }

        if (isCurrentNode)
        {
            float pulse = 0.5f + 0.5f * sinf(m_pulseTimer * 2.0f * 3.14159265f);
            uint32_t highlightColor = IM_COL32(255, 255, 0, static_cast<int>(pulse * 255));
            ImNodes::PushColorStyle(ImNodesCol_NodeOutline, highlightColor);
        }

        ImNodes::EndNode();

        if (isCurrentNode)
        {
            ImNodes::PopColorStyle();
        }

        ImNodes::SetNodeGridSpacePos(node->id, ImVec2(layout->position.x, layout->position.y));
    }

    void BehaviorTreeDebugWindow::RenderNodeConnections(const BTNode* node, const BTNodeLayout* layout, const BehaviorTreeAsset* tree)
    {
        static int lastFrameCleared = -1;
        static int currentFrame = 0;
        currentFrame++;

        if (m_editorMode && lastFrameCleared != currentFrame)
        {
            m_linkMap.clear();
            lastFrameCleared = currentFrame;
        }

        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence)
        {
            for (uint32_t childId : node->childIds)
            {
                int linkId = m_nextLinkId++;
                ImNodes::Link(linkId, node->id * 10000 + 1, childId * 10000);

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
        else if ((node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater) &&
            node->decoratorChildId != 0)
        {
            int linkId = m_nextLinkId++;
            ImNodes::Link(linkId, node->id * 10000 + 1, node->decoratorChildId * 10000);

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
            return IM_COL32(100, 150, 255, 255);
        case BTNodeType::Sequence:
            return IM_COL32(100, 255, 150, 255);
        case BTNodeType::Condition:
            return IM_COL32(255, 200, 100, 255);
        case BTNodeType::Action:
            return IM_COL32(255, 100, 150, 255);
        case BTNodeType::Inverter:
            return IM_COL32(200, 100, 255, 255);
        case BTNodeType::Repeater:
            return IM_COL32(150, 150, 255, 255);
        default:
            return IM_COL32(128, 128, 128, 255);
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

        if (ImGui::CollapsingHeader("Runtime Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderRuntimeInfo();
        }

        if (ImGui::CollapsingHeader("Blackboard", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderBlackboardSection();
        }

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

        if (ImGui::TreeNode("Movement"))
        {
            ImGui::Text("Has Move Goal: %s", blackboard.hasMoveGoal ? "Yes" : "No");
            ImGui::Text("Goal Position: (%.1f, %.1f)",
                blackboard.moveGoal.x,
                blackboard.moveGoal.y);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Patrol"))
        {
            ImGui::Text("Has Patrol Path: %s", blackboard.hasPatrolPath ? "Yes" : "No");
            ImGui::Text("Current Point: %d", blackboard.currentPatrolPoint);
            ImGui::Text("Point Count: %d", blackboard.patrolPointCount);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Combat"))
        {
            ImGui::Text("Can Attack: %s", blackboard.canAttack ? "Yes" : "No");
            ImGui::Text("Attack Cooldown: %.2f s", blackboard.attackCooldown);

            if (blackboard.lastAttackTime > 0.0f)
            {
                ImGui::Text("Last Attack Time: %.2f", blackboard.lastAttackTime);
            }
            else
            {
                ImGui::Text("Last Attack: Never");
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Stimuli"))
        {
            ImGui::Text("Heard Noise: %s", blackboard.heardNoise ? "Yes" : "No");
            ImGui::Text("Last Damage: %.2f", blackboard.damageAmount);
            ImGui::TreePop();
        }

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

        for (auto it = m_executionLog.rbegin(); it != m_executionLog.rend(); ++it)
        {
            const auto& entry = *it;

            if (entry.entity != m_selectedEntity)
                continue;

            ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            const char* icon = "▶";
            if (entry.status == BTStatus::Success)
            {
                color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                icon = "✓";
            }
            else if (entry.status == BTStatus::Failure)
            {
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
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

        Vector minPos, maxPos;
        GetGraphBounds(minPos, maxPos);

        Vector graphSize(maxPos.x - minPos.x, maxPos.y - minPos.y);
        ImVec2 imvectmp = ImGui::GetContentRegionAvail();
        Vector viewportSize = Vector(imvectmp.x, imvectmp.y);

        if (graphSize.x <= 0.0f || graphSize.y <= 0.0f)
        {
            CenterViewOnGraph();
            return;
        }

        float zoomX = viewportSize.x / graphSize.x;
        float zoomY = viewportSize.y / graphSize.y;
        float targetZoom = std::min(zoomX, zoomY) * 0.9f;

        m_currentZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, targetZoom));
        ApplyZoomToStyle();

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

        m_currentZoom = 1.0f;
        ApplyZoomToStyle();
        std::cout << "[BTDebugger] Reset zoom to 100%" << std::endl;
    }

    void BehaviorTreeDebugWindow::RenderMinimap()
    {
        if (m_currentLayout.empty())
            return;

        const ImVec2 minimapSize(200, 150);
        const ImVec2 minimapPadding(10, 10);

        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 minimapPos(
            contentMax.x - minimapSize.x - minimapPadding.x,
            contentMax.y - minimapSize.y - minimapPadding.y
        );

        ImGui::SetCursorPos(minimapPos);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 minimapMin = ImGui::GetCursorScreenPos();
        ImVec2 minimapMax(minimapMin.x + minimapSize.x, minimapMin.y + minimapSize.y);

        drawList->AddRectFilled(minimapMin, minimapMax, IM_COL32(20, 20, 20, 200), 4.0f);

        Vector graphMin, graphMax;
        GetGraphBounds(graphMin, graphMax);

        Vector graphSize(graphMax.x - graphMin.x, graphMax.y - graphMin.y);

        if (graphSize.x <= 0.0f || graphSize.y <= 0.0f)
        {
            ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
            ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
            return;
        }

        float scaleX = minimapSize.x / graphSize.x;
        float scaleY = minimapSize.y / graphSize.y;
        float scale = std::min(scaleX, scaleY) * 0.9f;

        if (scale <= 0.0f)
        {
            ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
            ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
            return;
        }

        for (const auto& layout : m_currentLayout)
        {
            float x = minimapMin.x + (layout.position.x - graphMin.x) * scale;
            float y = minimapMin.y + (layout.position.y - graphMin.y) * scale;

            ImU32 color = IM_COL32(100, 150, 255, 255);

            auto& world = World::Get();
            if (m_selectedEntity != 0 && world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
            {
                const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
                if (layout.nodeId == btRuntime.AICurrentNodeIndex)
                    color = IM_COL32(255, 255, 0, 255);
            }

            drawList->AddCircleFilled(ImVec2(x, y), 3.0f, color);
        }

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
            IM_COL32(255, 0, 0, 150),
            0.0f,
            0,
            2.0f
        );

        ImGui::SetCursorPos(ImVec2(minimapPos.x + 5, minimapPos.y + 5));
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Minimap");
    }

    void BehaviorTreeDebugWindow::RenderEditorToolbar()
    {
        if (ImGui::Button("Add Node"))
        {
            m_showNodePalette = true;
            m_nodeCreationPos.Set(ImGui::GetMousePos().x, ImGui::GetMousePos().y, 0.f);
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
        BTNode newNode;
        newNode.type = nodeType;
        newNode.id = m_nextNodeId++;

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
                m_editingTree.id = btRuntime.AITreeAssetId;
                m_editingTree.name = "New Tree";
                m_editingTree.rootNodeId = 0;
            }
        }

        m_editingTree.nodes.push_back(newNode);

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

        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);

        std::cout << "[BTEditor] Created node: " << newNode.name << " (ID: " << newNode.id << ")" << std::endl;
    }

    void BehaviorTreeDebugWindow::HandleNodeDeletion()
    {
        if (m_selectedNodes.empty())
            return;

        for (uint32_t nodeId : m_selectedNodes)
        {
            auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                [nodeId](const BTNode& n) { return n.id == nodeId; });

            if (it != m_editingTree.nodes.end())
            {
                EditorAction action;
                action.type = EditorAction::DeleteNode;
                action.nodeData = *it;
                m_undoStack.push_back(action);
                if (m_undoStack.size() > kMaxUndoStackSize)
                {
                    m_undoStack.erase(m_undoStack.begin());
                }

                m_editingTree.nodes.erase(it);

                for (auto& node : m_editingTree.nodes)
                {
                    auto childIt = std::find(node.childIds.begin(), node.childIds.end(), nodeId);
                    if (childIt != node.childIds.end())
                    {
                        node.childIds.erase(childIt);
                    }

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

        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
    }

    void BehaviorTreeDebugWindow::HandleNodeDuplication()
    {
        if (m_selectedNodes.empty())
            return;

        std::vector<uint32_t> newNodes;

        for (uint32_t nodeId : m_selectedNodes)
        {
            auto it = std::find_if(m_editingTree.nodes.begin(), m_editingTree.nodes.end(),
                [nodeId](const BTNode& n) { return n.id == nodeId; });

            if (it != m_editingTree.nodes.end())
            {
                BTNode duplicate = *it;
                duplicate.id = m_nextNodeId++;
                duplicate.name = duplicate.name + " (Copy)";

                m_editingTree.nodes.push_back(duplicate);
                newNodes.push_back(duplicate.id);

                EditorAction action;
                action.type = EditorAction::AddNode;
                action.nodeData = duplicate;
                m_undoStack.push_back(action);

                std::cout << "[BTEditor] Duplicated node: " << duplicate.name << " (ID: " << duplicate.id << ")" << std::endl;
            }
        }

        m_selectedNodes = newNodes;
        m_redoStack.clear();
        m_treeModified = true;

        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
    }

    bool BehaviorTreeDebugWindow::ValidateConnection(uint32_t parentId, uint32_t childId) const
    {
        const BTNode* parent = m_editingTree.GetNode(parentId);
        const BTNode* child = m_editingTree.GetNode(childId);

        if (!parent || !child)
            return false;

        if (parentId == childId)
            return false;

        if (parent->type != BTNodeType::Selector &&
            parent->type != BTNodeType::Sequence &&
            parent->type != BTNodeType::Inverter &&
            parent->type != BTNodeType::Repeater)
        {
            return false;
        }

        if ((parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater) &&
            parent->decoratorChildId != 0)
        {
            return false;
        }

        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
        {
            if (std::find(parent->childIds.begin(), parent->childIds.end(), childId) != parent->childIds.end())
            {
                return false;
            }
        }

        std::vector<uint32_t> visited;
        std::vector<uint32_t> toVisit;
        toVisit.push_back(childId);

        while (!toVisit.empty())
        {
            uint32_t currentId = toVisit.back();
            toVisit.pop_back();

            if (currentId == parentId)
            {
                return false;
            }

            if (std::find(visited.begin(), visited.end(), currentId) != visited.end())
            {
                continue;
            }

            visited.push_back(currentId);

            const BTNode* current = m_editingTree.GetNode(currentId);
            if (current)
            {
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

        json treeJson;
        treeJson["schema_version"] = 2;
        treeJson["type"] = "BehaviorTree";
        treeJson["blueprintType"] = "BehaviorTree";
        treeJson["name"] = m_editingTree.name;
        treeJson["description"] = "Edited in BT Editor";

        json metadata;
        metadata["author"] = "BT Editor";

        auto now = std::time(nullptr);
        char timestamp[32];
        std::tm timeInfo;

#ifdef _WIN32
        localtime_s(&timeInfo, &now);
#else
        localtime_r(&now, &timeInfo);
#endif

        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeInfo);
        metadata["created"] = timestamp;
        metadata["lastModified"] = timestamp;

        json tagsArray = json::array();
        tagsArray.push_back("AI");
        tagsArray.push_back("BehaviorTree");
        tagsArray.push_back("Edited");
        metadata["tags"] = tagsArray;

        treeJson["metadata"] = metadata;

        json editorState;
        editorState["zoom"] = 1.0;
        editorState["scrollOffset"] = { {"x", 0}, {"y", 0} };
        treeJson["editorState"] = editorState;

        json dataSection;
        dataSection["rootNodeId"] = static_cast<int>(m_editingTree.rootNodeId);

        json nodesArray = json::array();
        for (const auto& node : m_editingTree.nodes)
        {
            json nodeJson;
            nodeJson["id"] = static_cast<int>(node.id);
            nodeJson["name"] = node.name;

            switch (node.type)
            {
            case BTNodeType::Selector: nodeJson["type"] = "Selector"; break;
            case BTNodeType::Sequence: nodeJson["type"] = "Sequence"; break;
            case BTNodeType::Condition: nodeJson["type"] = "Condition"; break;
            case BTNodeType::Action: nodeJson["type"] = "Action"; break;
            case BTNodeType::Inverter: nodeJson["type"] = "Inverter"; break;
            case BTNodeType::Repeater: nodeJson["type"] = "Repeater"; break;
            }

            nodeJson["position"] = { {"x", 0.0}, {"y", 0.0} };

            if (node.type == BTNodeType::Condition)
            {
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
                    nodeJson["parameters"] = { {"param", node.conditionParam} };
                }
                else
                {
                    nodeJson["parameters"] = json::object();
                }
            }
            else if (node.type == BTNodeType::Action)
            {
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

                json params = json::object();
                if (node.actionParam1 != 0.0f) params["param1"] = node.actionParam1;
                if (node.actionParam2 != 0.0f) params["param2"] = node.actionParam2;
                nodeJson["parameters"] = params;
            }
            else if (node.type == BTNodeType::Repeater)
            {
                nodeJson["repeatCount"] = node.repeatCount;
            }

            if (!node.childIds.empty())
            {
                json childIdsArray = json::array();
                for (uint32_t cid : node.childIds)
                    childIdsArray.push_back(static_cast<int>(cid));
                nodeJson["childIds"] = childIdsArray;
            }
            if (node.decoratorChildId != 0)
            {
                nodeJson["decoratorChildId"] = static_cast<int>(node.decoratorChildId);
            }

            nodesArray.push_back(nodeJson);
        }

        dataSection["nodes"] = nodesArray;
        treeJson["data"] = dataSection;

        std::string filename = "Blueprints/AI/" + m_editingTree.name + "_edited.json";

        try
        {
            std::ofstream file(filename);
            if (file.is_open())
            {
                file << treeJson.dump(2);
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

        switch (action.type)
        {
        case EditorAction::AddNode:
        {
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
            m_editingTree.nodes.push_back(action.nodeData);
            break;
        }
        case EditorAction::AddConnection:
        {
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

        m_redoStack.push_back(action);

        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);

        std::cout << "[BTEditor] Undo performed" << std::endl;
    }

    void BehaviorTreeDebugWindow::RedoLastAction()
    {
        if (m_redoStack.empty())
            return;

        EditorAction action = m_redoStack.back();
        m_redoStack.pop_back();

        switch (action.type)
        {
        case EditorAction::AddNode:
        {
            m_editingTree.nodes.push_back(action.nodeData);
            break;
        }
        case EditorAction::DeleteNode:
        {
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

        m_undoStack.push_back(action);
        if (m_undoStack.size() > kMaxUndoStackSize)
        {
            m_undoStack.erase(m_undoStack.begin());
        }

        m_currentLayout = m_layoutEngine.ComputeLayout(&m_editingTree, m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);

        std::cout << "[BTEditor] Redo performed" << std::endl;
    }

    void BehaviorTreeDebugWindow::LoadBTConfig()
    {
        json configJson;
        if (!JsonHelper::LoadJsonFromFile("Config/BT_config.json", configJson))
        {
            std::cerr << "[BTDebugger] Failed to load BT_config.json, using defaults" << std::endl;
            m_configLoaded = false;
            return;
        }

        if (JsonHelper::IsObject(configJson, "layout"))
        {
            const auto& layout = configJson["layout"];
            m_config.defaultHorizontal = JsonHelper::GetString(layout, "defaultDirection", "horizontal") == "horizontal";
            m_config.gridSize = JsonHelper::GetFloat(layout, "gridSize", 16.0f);
            m_config.gridSnappingEnabled = JsonHelper::GetBool(layout, "gridSnappingEnabled", true);
            m_config.horizontalSpacing = JsonHelper::GetFloat(layout, "horizontalSpacing", 280.0f);
            m_config.verticalSpacing = JsonHelper::GetFloat(layout, "verticalSpacing", 120.0f);
        }

        if (JsonHelper::IsObject(configJson, "rendering"))
        {
            const auto& rendering = configJson["rendering"];
            m_config.pinRadius = JsonHelper::GetFloat(rendering, "pinRadius", 6.0f);
            m_config.pinOutlineThickness = JsonHelper::GetFloat(rendering, "pinOutlineThickness", 2.0f);
            m_config.bezierTangent = JsonHelper::GetFloat(rendering, "bezierTangent", 80.0f);
            m_config.connectionThickness = JsonHelper::GetFloat(rendering, "connectionThickness", 2.0f);
        }

        if (JsonHelper::IsObject(configJson, "nodeColors"))
        {
            const auto& nodeColors = configJson["nodeColors"];

            std::vector<std::string> nodeTypes = { "Selector", "Sequence", "Condition", "Action", "Inverter", "Repeater" };
            std::vector<std::string> statusTypes = { "idle", "running", "success", "failure", "aborted" };

            for (const auto& nodeType : nodeTypes)
            {
                if (JsonHelper::IsObject(nodeColors, nodeType))
                {
                    const auto& typeColors = nodeColors[nodeType];

                    for (const auto& status : statusTypes)
                    {
                        if (JsonHelper::IsObject(typeColors, status))
                        {
                            const auto& colorObj = typeColors[status];
                            BTConfig::Color color;
                            color.r = static_cast<uint8_t>(JsonHelper::GetInt(colorObj, "r", 128));
                            color.g = static_cast<uint8_t>(JsonHelper::GetInt(colorObj, "g", 128));
                            color.b = static_cast<uint8_t>(JsonHelper::GetInt(colorObj, "b", 128));
                            color.a = static_cast<uint8_t>(JsonHelper::GetInt(colorObj, "a", 255));

                            m_config.nodeColors[nodeType][status] = color;
                        }
                    }
                }
            }
        }

        if (configJson.contains("nodeColors") && configJson["nodeColors"].is_object())
        {
            const auto& colorsJson = configJson["nodeColors"];

            std::map<std::string, BTNodeType> typeMap;
            typeMap["Selector"] = BTNodeType::Selector;
            typeMap["Sequence"] = BTNodeType::Sequence;
            typeMap["Action"] = BTNodeType::Action;
            typeMap["Condition"] = BTNodeType::Condition;
            typeMap["Inverter"] = BTNodeType::Inverter;
            typeMap["Repeater"] = BTNodeType::Repeater;

            std::map<std::string, BTStatus> statusMap;
            statusMap["idle"] = BTStatus::Idle;
            statusMap["running"] = BTStatus::Running;
            statusMap["success"] = BTStatus::Success;
            statusMap["failure"] = BTStatus::Failure;
            statusMap["aborted"] = BTStatus::Aborted;

            for (auto typeIt = colorsJson.begin(); typeIt != colorsJson.end(); ++typeIt)
            {
                const std::string typeName = typeIt.key();
                const auto& statusColors = typeIt.value();

                auto typeMapIt = typeMap.find(typeName);
                if (typeMapIt == typeMap.end())
                    continue;

                BTNodeType nodeType = typeMapIt->second;

                for (auto statusIt = statusColors.begin(); statusIt != statusColors.end(); ++statusIt)
                {
                    const std::string statusName = statusIt.key();
                    const auto& colorJson = statusIt.value();

                    auto statusMapIt = statusMap.find(statusName);
                    if (statusMapIt == statusMap.end())
                        continue;

                    BTStatus status = statusMapIt->second;

                    BTColor color;
                    color.r = static_cast<uint8_t>(JsonHelper::GetInt(colorJson, "r", 255));
                    color.g = static_cast<uint8_t>(JsonHelper::GetInt(colorJson, "g", 255));
                    color.b = static_cast<uint8_t>(JsonHelper::GetInt(colorJson, "b", 255));
                    color.a = static_cast<uint8_t>(JsonHelper::GetInt(colorJson, "a", 255));

                    m_nodeColors[nodeType][status] = color;
                }
            }

            std::cout << "[BTDebugger] Loaded " << m_nodeColors.size() << " node color schemes" << std::endl;
        }

        m_configLoaded = true;
        std::cout << "[BTDebugger] Configuration loaded from BT_config.json" << std::endl;
    }

    void BehaviorTreeDebugWindow::ApplyConfigToLayout()
    {
        if (!m_configLoaded)
            return;

        m_layoutDirection = m_config.defaultHorizontal ? BTLayoutDirection::LeftToRight : BTLayoutDirection::TopToBottom;
        m_layoutEngine.SetLayoutDirection(m_layoutDirection);

        m_nodeSpacingX = m_config.horizontalSpacing;
        m_nodeSpacingY = m_config.verticalSpacing;

        std::cout << "[BTDebugger] Applied configuration to layout engine" << std::endl;
    }

    Vector BehaviorTreeDebugWindow::SnapToGrid(const Vector& pos) const
    {
        if (!m_config.gridSnappingEnabled)
            return pos;

        float gridSize = m_config.gridSize;
        return Vector(
            std::round(pos.x / gridSize) * gridSize,
            std::round(pos.y / gridSize) * gridSize,
            pos.z
        );
    }

    void BehaviorTreeDebugWindow::RenderBezierConnection(const Vector& start, const Vector& end, uint32_t color, float thickness, float tangent)
    {
        ImVec2 p1(start.x, start.y);
        ImVec2 p2(end.x, end.y);

        ImVec2 cp1(p1.x + tangent, p1.y);
        ImVec2 cp2(p2.x - tangent, p2.y);

        ImGui::GetWindowDrawList()->AddBezierCubic(p1, cp1, cp2, p2, color, thickness);
    }

    void BehaviorTreeDebugWindow::RenderNodePins(const BTNode* node, const BTNodeLayout* layout)
    {
        if (!node || !layout)
            return;

        float halfWidth = layout->width / 2.0f;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (node->id != 0)
        {
            Vector inputPinPos(layout->position.x - halfWidth, layout->position.y, 0.0f);
            ImVec2 pinCenter(inputPinPos.x, inputPinPos.y);
            uint32_t pinColor = IM_COL32(200, 200, 200, 255);
            uint32_t outlineColor = IM_COL32(80, 80, 80, 255);

            drawList->AddCircleFilled(pinCenter, m_config.pinRadius + m_config.pinOutlineThickness, outlineColor);
            drawList->AddCircleFilled(pinCenter, m_config.pinRadius, pinColor);
        }

        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence ||
            node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater)
        {
            Vector outputPinPos(layout->position.x + halfWidth, layout->position.y, 0.0f);
            ImVec2 pinCenter(outputPinPos.x, outputPinPos.y);
            uint32_t pinColor = IM_COL32(200, 200, 200, 255);
            uint32_t outlineColor = IM_COL32(80, 80, 80, 255);

            drawList->AddCircleFilled(pinCenter, m_config.pinRadius + m_config.pinOutlineThickness, outlineColor);
            drawList->AddCircleFilled(pinCenter, m_config.pinRadius, pinColor);
        }
    }

    uint32_t BehaviorTreeDebugWindow::GetNodeColorByStatus(BTNodeType type, BTStatus status) const
    {
        if (!m_configLoaded)
        {
            return GetNodeColor(type);
        }

        auto typeIt = m_nodeColors.find(type);
        if (typeIt != m_nodeColors.end())
        {
            const auto& statusColors = typeIt->second;
            auto statusIt = statusColors.find(status);
            if (statusIt != statusColors.end())
            {
                const BTColor& color = statusIt->second;
                return IM_COL32(color.r, color.g, color.b, color.a);
            }
        }

        return GetNodeColor(type);
    }
    
    // =============================================================================
    // Validation Panel
    // =============================================================================
    
    void BehaviorTreeDebugWindow::RenderValidationPanel()
    {
        if (!m_showValidationPanel || !m_editorMode)
            return;
        
        ImGui::Separator();
        ImGui::Text("Validation (%zu messages)", m_validationMessages.size());
        
        if (ImGui::BeginChild("ValidationMessages", ImVec2(0, 150), true))
        {
            for (const auto& msg : m_validationMessages)
            {
                ImVec4 color;
                const char* icon;
                
                if (msg.severity == BTValidationMessage::Severity::Error)
                {
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    icon = "[ERROR]";
                }
                else if (msg.severity == BTValidationMessage::Severity::Warning)
                {
                    color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                    icon = "[WARN]";
                }
                else
                {
                    color = ImVec4(0.3f, 0.8f, 1.0f, 1.0f);
                    icon = "[INFO]";
                }
                
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%s Node %u: %s", icon, msg.nodeId, msg.message.c_str());
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild();
    }
    
    uint32_t BehaviorTreeDebugWindow::GetPinColor(uint32_t nodeId, PinType pinType) const
    {
        // Check validation messages for this node
        for (const auto& msg : m_validationMessages)
        {
            if (msg.nodeId == nodeId)
            {
                if (msg.severity == BTValidationMessage::Severity::Error)
                {
                    return IM_COL32(255, 80, 80, 255); // Red
                }
                else if (msg.severity == BTValidationMessage::Severity::Warning)
                {
                    return IM_COL32(255, 200, 80, 255); // Yellow
                }
            }
        }
        
        return IM_COL32(80, 255, 80, 255); // Green - valid
    }
    
    bool BehaviorTreeDebugWindow::IsConnectionValid(uint32_t parentId, uint32_t childId) const
    {
        if (!m_editorMode)
            return false;
        
        // Use the validation method from BehaviorTree
        const BTNode* parent = m_editingTree.GetNode(parentId);
        const BTNode* child = m_editingTree.GetNode(childId);
        
        if (!parent || !child)
            return false;
        
        // Check if parent can have children
        if (parent->type != BTNodeType::Selector &&
            parent->type != BTNodeType::Sequence &&
            parent->type != BTNodeType::Inverter &&
            parent->type != BTNodeType::Repeater)
        {
            return false;
        }
        
        // Check decorator constraint
        if ((parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater) &&
            parent->decoratorChildId != 0 && parent->decoratorChildId != childId)
        {
            return false;
        }
        
        // Check for cycles by creating a temporary connection
        BehaviorTreeAsset tempTree = m_editingTree;
        tempTree.ConnectNodes(parentId, childId);
        
        return !tempTree.DetectCycle(parentId);
    }
    
    // =============================================================================
    // Node Properties Editor
    // =============================================================================
    
    void BehaviorTreeDebugWindow::RenderNodeProperties()
    {
        if (!m_showNodeProperties || m_inspectedNodeId == 0)
            return;
        
        BTNode* node = m_editingTree.GetNode(m_inspectedNodeId);
        if (!node)
        {
            m_showNodeProperties = false;
            return;
        }
        
        ImGui::Separator();
        ImGui::Text("Node Properties");
        
        if (ImGui::BeginChild("NodeProperties", ImVec2(0, 300), true))
        {
            ImGui::Text("ID: %u", node->id);
            ImGui::Text("Type: %d", static_cast<int>(node->type));
            
            // Editable name
            char nameBuf[256];
            strncpy(nameBuf, node->name.c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            {
                node->name = nameBuf;
                m_isDirty = true;
            }
            
            // Type-specific parameters
            if (node->type == BTNodeType::Action)
            {
                ImGui::Separator();
                ImGui::Text("Action Parameters");
                
                // Action type dropdown
                const char* actionTypes[] = {
                    "SetMoveGoalToLastKnownTargetPos",
                    "SetMoveGoalToTarget",
                    "SetMoveGoalToPatrolPoint",
                    "MoveToGoal",
                    "AttackIfClose",
                    "PatrolPickNextPoint",
                    "ClearTarget",
                    "Idle",
                    "WaitRandomTime",
                    "ChooseRandomNavigablePoint",
                    "RequestPathfinding",
                    "FollowPath"
                };
                
                int currentAction = static_cast<int>(node->actionType);
                if (ImGui::Combo("Action Type", &currentAction, actionTypes, 12))
                {
                    node->actionType = static_cast<BTActionType>(currentAction);
                    m_isDirty = true;
                }
                
                // Parameters
                if (ImGui::InputFloat("Param 1", &node->actionParam1))
                {
                    m_isDirty = true;
                }
                
                if (ImGui::InputFloat("Param 2", &node->actionParam2))
                {
                    m_isDirty = true;
                }
            }
            else if (node->type == BTNodeType::Condition)
            {
                ImGui::Separator();
                ImGui::Text("Condition Parameters");
                
                // Condition type dropdown
                const char* conditionTypes[] = {
                    "TargetVisible",
                    "TargetInRange",
                    "HealthBelow",
                    "HasMoveGoal",
                    "CanAttack",
                    "HeardNoise",
                    "IsWaitTimerExpired",
                    "HasNavigableDestination",
                    "HasValidPath",
                    "HasReachedDestination"
                };
                
                int currentCondition = static_cast<int>(node->conditionType);
                if (ImGui::Combo("Condition Type", &currentCondition, conditionTypes, 10))
                {
                    node->conditionType = static_cast<BTConditionType>(currentCondition);
                    m_isDirty = true;
                }
                
                // Parameter
                if (ImGui::InputFloat("Param", &node->conditionParam))
                {
                    m_isDirty = true;
                }
            }
            else if (node->type == BTNodeType::Repeater)
            {
                ImGui::Separator();
                ImGui::Text("Repeater Parameters");
                
                if (ImGui::InputInt("Repeat Count", &node->repeatCount))
                {
                    m_isDirty = true;
                }
            }
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Close Properties"))
        {
            m_showNodeProperties = false;
        }
    }
    
    // =============================================================================
    // JSON Save System
    // =============================================================================
    
    std::string BehaviorTreeDebugWindow::GetCurrentTimestamp() const
    {
        time_t now = time(nullptr);
        struct tm timeinfo;
        
        #ifdef _WIN32
            localtime_s(&timeinfo, &now);
        #else
            localtime_r(&now, &timeinfo);
        #endif
        
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
        return std::string(buffer);
    }
    
    json BehaviorTreeDebugWindow::SerializeTreeToJson(const BehaviorTreeAsset& tree) const
    {
        json j = json::object();
        
        j["schema_version"] = 2;
        j["type"] = "BehaviorTree";
        j["blueprintType"] = "BehaviorTree";
        j["name"] = tree.name;
        j["description"] = "";
        
        // Metadata
        json metadata = json::object();
        metadata["author"] = "Atlasbruce";
        metadata["created"] = GetCurrentTimestamp();
        metadata["lastModified"] = GetCurrentTimestamp();
        
        json tags = json::array();
        tags.push_back("AI");
        tags.push_back("BehaviorTree");
        metadata["tags"] = tags;
        
        j["metadata"] = metadata;
        
        // Editor state
        json editorState = json::object();
        editorState["zoom"] = 1.0f;
        
        json scrollOffset = json::object();
        scrollOffset["x"] = 0.0f;
        scrollOffset["y"] = 0.0f;
        editorState["scrollOffset"] = scrollOffset;
        
        j["editorState"] = editorState;
        
        // Data
        json data = json::object();
        data["rootNodeId"] = tree.rootNodeId;
        
        json nodesArray = json::array();
        for (const auto& node : tree.nodes)
        {
            json nodeJson = json::object();
            nodeJson["id"] = node.id;
            nodeJson["name"] = node.name;
            
            // Node type
            const char* typeStr = "";
            switch (node.type)
            {
                case BTNodeType::Selector: typeStr = "Selector"; break;
                case BTNodeType::Sequence: typeStr = "Sequence"; break;
                case BTNodeType::Condition: typeStr = "Condition"; break;
                case BTNodeType::Action: typeStr = "Action"; break;
                case BTNodeType::Inverter: typeStr = "Inverter"; break;
                case BTNodeType::Repeater: typeStr = "Repeater"; break;
            }
            nodeJson["type"] = typeStr;
            
            // Position (placeholder - would need to get from layout)
            json pos = json::object();
            pos["x"] = 200.0f;
            pos["y"] = 100.0f * static_cast<float>(node.id);
            nodeJson["position"] = pos;
            
            // Children array (for composites)
            if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
            {
                json children = json::array();
                for (uint32_t childId : node.childIds)
                {
                    children.push_back(childId);
                }
                nodeJson["children"] = children;
            }
            
            // Decorator child
            if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
            {
                if (node.decoratorChildId != 0)
                {
                    nodeJson["decoratorChildId"] = node.decoratorChildId;
                }
                
                if (node.type == BTNodeType::Repeater)
                {
                    nodeJson["repeatCount"] = node.repeatCount;
                }
            }
            
            // Action type and parameters
            if (node.type == BTNodeType::Action)
            {
                const char* actionTypeStr = "";
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
                
                json params = json::object();
                params["param1"] = node.actionParam1;
                params["param2"] = node.actionParam2;
                nodeJson["parameters"] = params;
            }
            
            // Condition type and parameters
            if (node.type == BTNodeType::Condition)
            {
                const char* conditionTypeStr = "";
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
                
                json params = json::object();
                params["param"] = node.conditionParam;
                nodeJson["parameters"] = params;
            }
            
            // Default empty parameters if not set
            if (!nodeJson.contains("parameters"))
            {
                nodeJson["parameters"] = json::object();
            }
            
            nodesArray.push_back(nodeJson);
        }
        
        data["nodes"] = nodesArray;
        j["data"] = data;
        
        return j;
    }
    
    void BehaviorTreeDebugWindow::Save()
    {
        if (m_currentFilePath.empty())
        {
            SaveAs();
            return;
        }
        
        // Validate before saving
        m_validationMessages = m_editingTree.ValidateTreeFull();
        
        // Check for critical errors
        bool hasErrors = false;
        for (const auto& msg : m_validationMessages)
        {
            if (msg.severity == BTValidationMessage::Severity::Error)
            {
                hasErrors = true;
                break;
            }
        }
        
        if (hasErrors)
        {
            std::cout << "[BTEditor] Cannot save: tree has validation errors" << std::endl;
            return;
        }
        
        // Serialize and save
        json j = SerializeTreeToJson(m_editingTree);
        
        if (JsonHelper::SaveJsonToFile(m_currentFilePath, j, 2))
        {
            std::cout << "[BTEditor] Saved tree to: " << m_currentFilePath << std::endl;
            m_isDirty = false;
        }
        else
        {
            std::cout << "[BTEditor] Failed to save tree" << std::endl;
        }
    }
    
    void BehaviorTreeDebugWindow::SaveAs()
    {
        // Generate filename from tree name
        std::string filename = "Blueprints/AI/" + m_editingTree.name + "_edited.json";
        m_currentFilePath = filename;
        Save();
    }
    
    void BehaviorTreeDebugWindow::RenderFileMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New BT...", ""))
            {
                m_showNewBTDialog = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Save", "Ctrl+S", false, m_editorMode && m_isDirty))
            {
                Save();
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, m_editorMode))
            {
                SaveAs();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Close", "", false, m_editorMode))
            {
                // TODO: Add confirmation dialog if dirty
                m_editorMode = false;
            }
            
            ImGui::EndMenu();
        }
    }
    
    void BehaviorTreeDebugWindow::RenderEditMenu()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            bool canUndo = m_commandStack.CanUndo();
            bool canRedo = m_commandStack.CanRedo();
            
            std::string undoText = "Undo";
            if (canUndo)
            {
                undoText += " (" + m_commandStack.GetUndoDescription() + ")";
            }
            
            std::string redoText = "Redo";
            if (canRedo)
            {
                redoText += " (" + m_commandStack.GetRedoDescription() + ")";
            }
            
            if (ImGui::MenuItem(undoText.c_str(), "Ctrl+Z", false, canUndo))
            {
                m_commandStack.Undo();
                m_isDirty = true;
            }
            
            if (ImGui::MenuItem(redoText.c_str(), "Ctrl+Y", false, canRedo))
            {
                m_commandStack.Redo();
                m_isDirty = true;
            }
            
            ImGui::EndMenu();
        }
    }
    
    // =============================================================================
    // New BT from Template
    // =============================================================================
    
    BehaviorTreeAsset BehaviorTreeDebugWindow::CreateFromTemplate(int templateIndex, const std::string& name)
    {
        BehaviorTreeAsset tree;
        tree.name = name;
        tree.id = 9999; // Temporary ID
        
        if (templateIndex == 0)
        {
            // Empty template - just a root Selector
            BTNode root;
            root.type = BTNodeType::Selector;
            root.id = 1;
            root.name = "Root Selector";
            tree.nodes.push_back(root);
            tree.rootNodeId = 1;
        }
        else if (templateIndex == 1)
        {
            // Basic AI - idle + wander
            BTNode root;
            root.type = BTNodeType::Selector;
            root.id = 1;
            root.name = "Root Selector";
            root.childIds.push_back(2);
            tree.nodes.push_back(root);
            
            BTNode sequence;
            sequence.type = BTNodeType::Sequence;
            sequence.id = 2;
            sequence.name = "Wander Sequence";
            sequence.childIds.push_back(3);
            sequence.childIds.push_back(4);
            tree.nodes.push_back(sequence);
            
            BTNode wait;
            wait.type = BTNodeType::Action;
            wait.id = 3;
            wait.name = "Wait";
            wait.actionType = BTActionType::WaitRandomTime;
            wait.actionParam1 = 2.0f;
            wait.actionParam2 = 6.0f;
            tree.nodes.push_back(wait);
            
            BTNode choose;
            choose.type = BTNodeType::Action;
            choose.id = 4;
            choose.name = "Choose Point";
            choose.actionType = BTActionType::ChooseRandomNavigablePoint;
            choose.actionParam1 = 500.0f;
            choose.actionParam2 = 10.0f;
            tree.nodes.push_back(choose);
            
            tree.rootNodeId = 1;
        }
        else if (templateIndex == 2)
        {
            // Patrol template
            BTNode root;
            root.type = BTNodeType::Sequence;
            root.id = 1;
            root.name = "Patrol Sequence";
            root.childIds.push_back(2);
            root.childIds.push_back(3);
            root.childIds.push_back(4);
            tree.nodes.push_back(root);
            
            BTNode pick;
            pick.type = BTNodeType::Action;
            pick.id = 2;
            pick.name = "Pick Next Point";
            pick.actionType = BTActionType::PatrolPickNextPoint;
            tree.nodes.push_back(pick);
            
            BTNode setGoal;
            setGoal.type = BTNodeType::Action;
            setGoal.id = 3;
            setGoal.name = "Set Goal";
            setGoal.actionType = BTActionType::SetMoveGoalToPatrolPoint;
            tree.nodes.push_back(setGoal);
            
            BTNode move;
            move.type = BTNodeType::Action;
            move.id = 4;
            move.name = "Move";
            move.actionType = BTActionType::MoveToGoal;
            tree.nodes.push_back(move);
            
            tree.rootNodeId = 1;
        }
        else if (templateIndex == 3)
        {
            // Combat template
            BTNode root;
            root.type = BTNodeType::Selector;
            root.id = 1;
            root.name = "Root Selector";
            root.childIds.push_back(2);
            root.childIds.push_back(5);
            tree.nodes.push_back(root);
            
            // Combat branch
            BTNode combatSeq;
            combatSeq.type = BTNodeType::Sequence;
            combatSeq.id = 2;
            combatSeq.name = "Combat Sequence";
            combatSeq.childIds.push_back(3);
            combatSeq.childIds.push_back(4);
            tree.nodes.push_back(combatSeq);
            
            BTNode hasTarget;
            hasTarget.type = BTNodeType::Condition;
            hasTarget.id = 3;
            hasTarget.name = "Has Target";
            hasTarget.conditionType = BTConditionType::TargetVisible;
            tree.nodes.push_back(hasTarget);
            
            BTNode attack;
            attack.type = BTNodeType::Action;
            attack.id = 4;
            attack.name = "Attack";
            attack.actionType = BTActionType::AttackIfClose;
            tree.nodes.push_back(attack);
            
            // Wander branch
            BTNode wander;
            wander.type = BTNodeType::Action;
            wander.id = 5;
            wander.name = "Wander";
            wander.actionType = BTActionType::ChooseRandomNavigablePoint;
            wander.actionParam1 = 300.0f;
            tree.nodes.push_back(wander);
            
            tree.rootNodeId = 1;
        }
        
        return tree;
    }
    
    void BehaviorTreeDebugWindow::RenderNewBTDialog()
    {
        if (!m_showNewBTDialog)
            return;
        
        ImGui::OpenPopup("New Behavior Tree");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("New Behavior Tree", &m_showNewBTDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Create a new behavior tree from template");
            ImGui::Separator();
            
            ImGui::InputText("Name", m_newBTName, sizeof(m_newBTName));
            
            ImGui::Text("Template:");
            ImGui::RadioButton("Empty (root node only)", &m_selectedTemplate, 0);
            ImGui::RadioButton("Basic AI (idle + wander)", &m_selectedTemplate, 1);
            ImGui::RadioButton("Patrol (patrol points)", &m_selectedTemplate, 2);
            ImGui::RadioButton("Combat (combat + wander)", &m_selectedTemplate, 3);
            
            ImGui::Separator();
            
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                if (strlen(m_newBTName) > 0)
                {
                    m_editingTree = CreateFromTemplate(m_selectedTemplate, std::string(m_newBTName));
                    m_editorMode = true;
                    m_isDirty = true;
                    m_currentFilePath = "";
                    m_showNewBTDialog = false;
                    std::cout << "[BTEditor] Created new tree: " << m_newBTName << std::endl;
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_showNewBTDialog = false;
            }
            
            ImGui::EndPopup();
        }
    }

} // namespace Olympe