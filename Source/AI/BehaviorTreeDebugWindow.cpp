/**
 * @file BehaviorTreeDebugWindow.cpp
 * @brief Implementation of behavior tree runtime debugger
 */

#include "BehaviorTreeDebugWindow.h"
#include "../World.h"
#include "../GameEngine.h"
#include "../ECS_Components.h"
#include "../ECS_Components_AI.h"
#include "../json_helper.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <ctime>
#include <unordered_set>
#include <set>

namespace Olympe
{
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

        if (!m_imnodesInitialized)
        {
            ImNodes::CreateContext();
            ImNodes::GetStyle().GridSpacing = 32.0f;
            ImNodes::GetStyle().NodeCornerRounding = 8.0f;
            ImNodes::GetStyle().NodePadding = ImVec2(8, 8);
            m_imnodesInitialized = true;
        }

        m_isInitialized = true;

        // Set up autosave timing only.  The per-save lambda overload of
        // ScheduleSave() is used at each change site so that serialization
        // happens on the UI thread and the background task only does I/O.
        m_autosave.Init(nullptr, 1.5f, 60.0f);

        // Initialize NodeGraph debug panel (Blueprint Editor pipeline, Runtime mode)
        InitNodeGraphDebugMode();

        std::cout << "[BTDebugger] Initialized (window will be created on first F10)" << std::endl;
    }

    void BehaviorTreeDebugWindow::Shutdown()
    {
        // 1. Flush autosave first
        m_autosave.Flush();

        // 2. Shutdown NodeGraph BEFORE destroying window (needs ImGui/ImNodes contexts alive)
        ShutdownNodeGraphDebugMode();

        // 3. Destroy ImNodes context BEFORE destroying ImGui context
        if (m_imnodesInitialized)
        {
            ImNodes::DestroyContext();
            m_imnodesInitialized = false;
        }

        // 4. Destroy window and ImGui context LAST
        DestroySeparateWindow();

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

            // Shutdown backends BEFORE destroying context
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();

            // Destroy ImGui context
            ImGui::DestroyContext(m_separateImGuiContext);
            m_separateImGuiContext = nullptr;
        }

        // Restore context BEFORE destroying SDL resources
        if (previousContext != nullptr && previousContext != m_separateImGuiContext)
            ImGui::SetCurrentContext(previousContext);

        // Destroy SDL resources LAST
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

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Keyboard shortcuts
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
        RenderNodeGraphDebugPanel();
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

    // =========================================================================
    // Phase 50.1.4: Temporary stub methods (BehaviorTreeDebugWindow_NodeGraph.cpp pending fix)
    // These are empty stubs to keep the project compiling until NodeGraphPanel is restored.
    // =========================================================================

    void BehaviorTreeDebugWindow::InitNodeGraphDebugMode()
    {
        // Stub: Real implementation should be in BehaviorTreeDebugWindow_NodeGraph.cpp
    }

    void BehaviorTreeDebugWindow::ShutdownNodeGraphDebugMode()
    {
        // Stub: Real implementation should be in BehaviorTreeDebugWindow_NodeGraph.cpp
    }

    void BehaviorTreeDebugWindow::RenderNodeGraphDebugPanel()
    {
        // Stub: Real implementation should be in BehaviorTreeDebugWindow_NodeGraph.cpp
    }

}
