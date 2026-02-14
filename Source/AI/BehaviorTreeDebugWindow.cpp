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
#include <algorithm>
#include <cstring>
#include <cmath>
#include <unordered_set>

namespace Olympe
{
    BehaviorTreeDebugWindow::BehaviorTreeDebugWindow()
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

        // Initialize ImNodes context
        if (!m_imnodesInitialized)
        {
            ImNodes::CreateContext();
            ImNodes::GetStyle().GridSpacing = 32.0f;
            ImNodes::GetStyle().NodeCornerRounding = 8.0f;
            ImNodes::GetStyle().NodePadding = ImVec2(8, 8);
            m_imnodesInitialized = true;
        }

        m_isInitialized = true;
    }

    void BehaviorTreeDebugWindow::Shutdown()
    {
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
        
        if (m_isVisible && !m_isInitialized)
        {
            Initialize();
        }
    }

    void BehaviorTreeDebugWindow::Render()
    {
        if (!m_isVisible || !m_isInitialized)
            return;

        // Update pulse animation (using delta time)
        m_pulseTimer += GameEngine::fDt;

        // Auto-refresh entity list (accumulate time properly)
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

        // Configure as professional external-style window with generous size
        ImGui::SetNextWindowSize(ImVec2(1800, 1200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

        // Force external floating window (no docking, no bring to front on focus)
        ImGuiWindowFlags windowFlags = 
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        if (!ImGui::Begin("Behavior Tree Runtime Debugger", &m_isVisible, windowFlags))
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
                // Reasonable spacing ranges to prevent massive graphs
                ImGui::SliderFloat("Node Spacing X", &m_nodeSpacingX, 150.0f, 500.0f);
                ImGui::SliderFloat("Node Spacing Y", &m_nodeSpacingY, 100.0f, 400.0f);
                
                // Reset button to restore defaults
                if (ImGui::Button("Reset Spacing to Defaults"))
                {
                    m_nodeSpacingX = 250.0f;
                    m_nodeSpacingY = 180.0f;
                    
                    // Recompute layout with new spacing
                    if (m_selectedEntity != 0)
                    {
                        auto& world = World::Get();
                        if (world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
                        {
                            const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
                            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
                            if (tree)
                            {
                                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY);
                            }
                        }
                    }
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
                m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY);
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
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY);
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

            std::cout << "[BTDebugger] Graph bounds: (" << minPos.x << "," << minPos.y 
                      << ") to (" << maxPos.x << "," << maxPos.y << ")" << std::endl;
            std::cout << "[BTDebugger] Graph size: " << graphSize.x << "x" << graphSize.y << " pixels" << std::endl;
            std::cout << "[BTDebugger] Graph center: (" << graphCenter.x << "," << graphCenter.y << ")" << std::endl;

            // ✅ NEW: Center camera when entity changes
            if (m_lastCenteredEntity != m_selectedEntity)
            {
                // Center the camera on the graph
                ImVec2 editorSize = ImGui::GetContentRegionAvail();
                ImVec2 cameraOffset(
                    graphCenter.x - editorSize.x / 2.0f,
                    graphCenter.y - editorSize.y / 2.0f
                );
                
                ImNodes::EditorContextResetPanning(cameraOffset);
                
                std::cout << "[BTDebugger] ✅ Camera centered on graph" << std::endl;
                m_lastCenteredEntity = m_selectedEntity;
            }
        }

        // ✅ NEW: Mouse wheel zoom support
        // Manual zoom with style scaling (ImNodes v0.4 compatible)
        static float currentZoom = 1.0f;

        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            if (io.MouseWheel != 0.0f)
            {
                float zoomDelta = io.MouseWheel * 0.1f;  // 10% per wheel notch
                currentZoom = std::max(0.3f, std::min(3.0f, currentZoom + zoomDelta));
                
                // Scale ImNodes style for zoom effect
                ImNodes::GetStyle().NodePadding = ImVec2(8.0f * currentZoom, 8.0f * currentZoom);
                ImNodes::GetStyle().NodeCornerRounding = 8.0f * currentZoom;
                ImNodes::GetStyle().GridSpacing = 32.0f * currentZoom;
                
                std::cout << "[BTDebugger] Zoom: " << (int)(currentZoom * 100) << "%" << std::endl;
            }
        }

        RenderBehaviorTreeGraph();

        ImNodes::EndNodeEditor();
    }

    void BehaviorTreeDebugWindow::RenderBehaviorTreeGraph()
    {
        auto& world = World::Get();
        const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
        
        // FIXED: Use enhanced lookup
        const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

        if (!tree)
            return;

        uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;

        // Render all nodes
        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                bool isCurrentNode = (node.id == currentNodeId) && btRuntime.isActive;
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
        static int linkIdCounter = 100000;

        // Composite nodes
        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence)
        {
            for (uint32_t childId : node->childIds)
            {
                int linkId = linkIdCounter++;
                ImNodes::Link(linkId, node->id * 10000 + 1, childId * 10000);
            }
        }
        // Decorator nodes
        else if ((node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater) && 
                  node->decoratorChildId != 0)
        {
            int linkId = linkIdCounter++;
            ImNodes::Link(linkId, node->id * 10000 + 1, node->decoratorChildId * 10000);
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
}
