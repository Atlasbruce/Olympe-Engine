    /**
 * @file BehaviorTreeDebugWindow.cpp
 * @brief Implementation of behavior tree runtime debugger
 */

#include "BehaviorTreeDebugWindow.h"
#include "BehaviorTreeDebugAPI.h"
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
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>

// Global debugger instance (declared/defined in OlympeEngine.cpp)
extern Olympe::BehaviorTreeDebugWindow* g_btDebugWindow;

namespace Olympe
{

    // Pending execution entries buffer kept globally so runtime can emit
    // execution events even when the debug window is not yet created/visible.
    // Entries are flushed to the debug window when it becomes available.
    static std::deque<ExecutionLogEntry> s_pendingExecutionBuffer;
    static const size_t MAX_PENDING_EXEC_BUFFER = 4096;

    BehaviorTreeDebugWindow::BehaviorTreeDebugWindow()
        : m_separateWindow(nullptr)
        , m_separateRenderer(nullptr)
        , m_windowCreated(false)
        , m_separateImGuiContext(nullptr)
    {
    }

    std::vector<ExecutionLogEntry> BehaviorTreeDebugWindow::GetExecutionLogSnapshot(size_t maxEntries) const
    {
        std::vector<ExecutionLogEntry> out;
        out.reserve(std::min(maxEntries, m_executionLog.size()));
        // return most recent entries up to maxEntries (chronological: oldest->newest)
        size_t start = (m_executionLog.size() > maxEntries) ? (m_executionLog.size() - maxEntries) : 0;
        for (size_t i = start; i < m_executionLog.size(); ++i) out.push_back(m_executionLog[i]);
        return out;
    }

    size_t BehaviorTreeDebugWindow::GetPendingCount() const
    {
        return s_pendingExecutionBuffer.size();
    }

    void BehaviorTreeDebugWindow::FlushPendingExtern()
    {
        while (!s_pendingExecutionBuffer.empty())
        {
            const auto e = s_pendingExecutionBuffer.front();
            s_pendingExecutionBuffer.pop_front();
            AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status);
        }
    }



    BehaviorTreeDebugWindow::~BehaviorTreeDebugWindow()
    {
        // Ensure clean shutdown when destructor is called
        try { Shutdown(); } catch (...) { /* swallow exceptions in dtor */ }
        // Ensure we also destroy any leftover ImNodes global context we created as fallback
        if (m_imnodesContext)
        {
            try { ImNodes::DestroyContext(static_cast<ImNodesContext*>(m_imnodesContext)); } catch (...) {}
            m_imnodesContext = nullptr;
        }
    }

    void BehaviorTreeDebugWindow::Initialize()
    {
        if (m_isInitialized)
            return;

        // Ensure an ImNodes context exists before initializing NodeGraphPanel
        // NodeGraphPanel may call ImNodes APIs during Initialize(), so create
        // the context here to avoid assertions when the panel later renders.
        if (!m_imnodesContext)
        {
            m_imnodesContext = ImNodes::CreateContext();
        }

        // NodeGraphPanel now owns ImNodes editor contexts (EditorContextCreate/Free).
        // Avoid creating a global ImNodes context here to prevent conflicts.

        m_isInitialized = true;

        // Set up autosave timing only.  The per-save lambda overload of
        // ScheduleSave() is used at each change site so that serialization
        // happens on the UI thread and the background task only does I/O.
        m_autosave.Init(nullptr, 1.5f, 60.0f);

        // Initialize NodeGraph debug panel (Blueprint Editor pipeline, Runtime mode)
        InitNodeGraphDebugMode();

        // Flush any pending execution entries recorded before the window existed
        if (!s_pendingExecutionBuffer.empty())
        {
            std::cout << "[BTDebugger] Flushing " << s_pendingExecutionBuffer.size() << " pending execution entries" << std::endl;
            for (const auto& e : s_pendingExecutionBuffer)
            {
                AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status);
                if (!e.rawJson.empty() && !m_executionLog.empty())
                {
                    m_executionLog.back().rawJson = e.rawJson;
                }
            }
            s_pendingExecutionBuffer.clear();
        }

        std::cout << "[BTDebugger] Initialized (window will be created on first F10)" << std::endl;
    }

    void BehaviorTreeDebugWindow::Shutdown()
    {
        // Make Shutdown idempotent: if not initialized, nothing to do
        if (!m_isInitialized)
            return;
        // 1. Flush autosave first
        m_autosave.Flush();

        // 2. Shutdown NodeGraph BEFORE destroying window (needs ImGui/ImNodes contexts alive)
        ShutdownNodeGraphDebugMode();

        // ImNodes editor contexts are owned/managed by NodeGraphPanel

        // 3. Destroy window and ImGui context LAST
        DestroySeparateWindow();

        m_isInitialized = false;

        std::cout << "[BTDebugger] Shutdown complete" << std::endl;
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

            // If there are pending entries, flush them now that the window is visible
            if (!s_pendingExecutionBuffer.empty())
            {
                size_t moved = 0;
                std::cout << "[BTDebugger] Window opened - auto-flushing " << s_pendingExecutionBuffer.size() << " pending entries" << std::endl;
                while (!s_pendingExecutionBuffer.empty())
                {
                    const auto e = s_pendingExecutionBuffer.front();
                    s_pendingExecutionBuffer.pop_front();
                    AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status);
                    // preserve rawJson if present
                    if (!e.rawJson.empty() && !m_executionLog.empty())
                        m_executionLog.back().rawJson = e.rawJson;
                    ++moved;
                }
                std::cout << "[BTDebugger] Auto-flushed " << moved << " entries" << std::endl;
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

        // Create a dedicated ImNodes context for the separate window's node graph
        if (!m_imnodesContext)
        {
            m_imnodesContext = ImNodes::CreateContext();
            // ImNodes::CreateContext returns an ImNodesContext*; store as void* to avoid header inclusion issues
        }

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

        // Destroy ImNodes context created for this separate window
        if (m_imnodesContext)
        {
            // ImNodes::DestroyContext expects ImNodesContext*
            ImNodes::DestroyContext(static_cast<ImNodesContext*>(m_imnodesContext));
            m_imnodesContext = nullptr;
        }

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

        // Draggable resize grip placed between node graph and inspector for clearer UX
        ImGui::PushID("InspectorResizeGrip");
        ImGui::InvisibleButton("##InspectorGrip", ImVec2(12.0f, windowHeight));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            float delta = ImGui::GetIO().MouseDelta.x;
            m_inspectorWidth -= delta; // dragging left increases center, so subtract
            if (m_inspectorWidth < 200.0f) m_inspectorWidth = 200.0f;
            if (m_inspectorWidth > 900.0f) m_inspectorWidth = 900.0f;
        }
        // visual separator
        ImGui::SameLine();

        // Make the inspector panel resizable horizontally by using a vertical splitter
        ImGui::BeginChild("InspectorPanel", ImVec2(m_inspectorWidth, windowHeight), true);
        RenderInspectorPanel();
        ImGui::EndChild();
        ImGui::PopID();

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
        // Row 1: primary actions + counters
        if (ImGui::Button("Clear Log"))
        {
            m_executionLog.clear();
        }
        ImGui::SameLine();
        ImGui::Text("Entries: %zu", m_executionLog.size());
        ImGui::SameLine();
        ImGui::Text("Pending: %zu", s_pendingExecutionBuffer.size());
        ImGui::SameLine();
        ImGui::Text("Selected Entity: %llu", static_cast<unsigned long long>(m_selectedEntity));

        // Row 2: secondary actions
        if (ImGui::SmallButton("Show all entities")) m_selectedEntity = 0;
        ImGui::SameLine();
        if (ImGui::SmallButton("Flush pending"))
        {
            // Move pending entries into the visible execution log
            size_t moved = 0;
            while (!s_pendingExecutionBuffer.empty())
            {
                const auto e = s_pendingExecutionBuffer.front();
                s_pendingExecutionBuffer.pop_front();
                AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status);
                ++moved;
            }
            std::cout << "[BTDebugger] Flushed " << moved << " pending entries" << std::endl;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(m_showHistoryWindow ? "Compact View" : "Full History"))
        {
            m_showHistoryWindow = !m_showHistoryWindow;
        }

        ImGui::Separator();

        // Two modes: compact log (current) and full-history viewer
        ImGui::BeginChild("ExecutionLogScroll", ImVec2(0, 0), false);

        if (!m_showHistoryWindow)
        {
            // Compact reverse-chronological view (most recent first)
            size_t displayIdx = 0;
            for (auto it = m_executionLog.rbegin(); it != m_executionLog.rend(); ++it)
            {
                const auto& entry = *it;

                if (m_selectedEntity != 0 && entry.entity != m_selectedEntity)
                    continue;

                ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                const char* icon = "▶";
                if (entry.status == BTStatus::Success) { color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); icon = "✓"; }
                else if (entry.status == BTStatus::Failure) { color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); icon = "✗"; }

                ImGui::TextColored(color, "[%.2fs ago] %s Node %u (%s)", entry.timeAgo, icon, entry.nodeId, entry.nodeName.c_str());
                if (entry.entity != 0) { ImGui::SameLine(); ImGui::Text("Entity: %llu", static_cast<unsigned long long>(entry.entity)); }

                if (!entry.rawJson.empty())
                {
                    // Use unique ID per displayed entry to avoid ImGui ID conflicts
                    std::string hdr = std::string("Details##") + std::to_string(displayIdx++);
                    if (ImGui::CollapsingHeader(hdr.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        try
                        {
                            auto j = nlohmann::json::parse(entry.rawJson);
                            if (j.contains("msg") && j["msg"].is_string()) ImGui::TextWrapped("Msg: %s", j["msg"].get<std::string>().c_str());
                            if (j.contains("entityName") && j["entityName"].is_string()) ImGui::Text("EntityName: %s", j["entityName"].get<std::string>().c_str());
                            if (j.contains("details") && j["details"].is_object())
                            {
                                ImGui::Separator(); ImGui::Text("Details:");
                                for (auto it = j["details"].begin(); it != j["details"].end(); ++it)
                                {
                                    const std::string key = it.key(); const auto& val = it.value();
                                    if (val.is_string()) ImGui::Text("  %s: %s", key.c_str(), val.get<std::string>().c_str());
                                    else if (val.is_number()) ImGui::Text("  %s: %g", key.c_str(), val.get<double>());
                                    else if (val.is_boolean()) ImGui::Text("  %s: %s", key.c_str(), val.get<bool>() ? "true" : "false");
                                    else ImGui::TextWrapped("  %s: %s", key.c_str(), val.dump().c_str());
                                }
                            }
                        }
                        catch (...) { ImGui::Text("(invalid JSON payload)"); }
                    }
                }
            }
        }
        else
        {
            // Full history viewer: group by entity and show chronological order
            // Provide a simple tree where each entity expands into its full recorded history.
            // Build a temporary map of entity -> vector of indices
            std::unordered_map<EntityID, std::vector<size_t>> idxMap;
            idxMap.reserve(m_executionLog.size());
            for (size_t i = 0; i < m_executionLog.size(); ++i)
            {
                const auto& e = m_executionLog[i];
                if (m_selectedEntity != 0 && e.entity != m_selectedEntity) continue;
                idxMap[e.entity].push_back(i);
            }

            for (auto itMap = idxMap.begin(); itMap != idxMap.end(); ++itMap)
            {
                EntityID ent = itMap->first;
                char label[64]; sprintf_s(label, sizeof(label), "Entity %llu", static_cast<unsigned long long>(ent));
                if (ImGui::TreeNode(label))
                {
                    // chronological from oldest to newest
                    for (size_t idx : itMap->second)
                    {
                        const auto& entry = m_executionLog[idx];
                        ImGui::Text("Node %u (%s)  [%s]", entry.nodeId, entry.nodeName.c_str(), (entry.status==BTStatus::Success)?"Success":(entry.status==BTStatus::Failure)?"Failure":"Running");
                    }
                    ImGui::TreePop();
                }
            }
        }

        ImGui::EndChild();
    }

    void BehaviorTreeDebugWindow::AddExecutionEntry(EntityID entity, uint32_t nodeId, const std::string& nodeName, BTStatus status)
    {
        static std::unordered_map<EntityID, std::deque<uint32_t>> s_executionHistory;
        static std::deque<EntityID> s_historyEntityOrder;
        constexpr size_t MAX_HISTORY_ENTITIES = 256;
        constexpr size_t MAX_HISTORY_PER_ENTITY = 64;

        auto historyIt = s_executionHistory.find(entity);
        if (historyIt == s_executionHistory.end())
        {
            s_executionHistory.emplace(entity, std::deque<uint32_t>{});
            s_historyEntityOrder.push_back(entity);

            while (s_historyEntityOrder.size() > MAX_HISTORY_ENTITIES)
            {
                EntityID oldestEntity = s_historyEntityOrder.front();
                s_historyEntityOrder.pop_front();
                s_executionHistory.erase(oldestEntity);
            }

            historyIt = s_executionHistory.find(entity);
        }

        std::deque<uint32_t>& entityHistory = historyIt->second;
        entityHistory.push_back(nodeId);
        while (entityHistory.size() > MAX_HISTORY_PER_ENTITY)
        {
            entityHistory.pop_front();
        }

        ExecutionLogEntry entry;
        entry.timeAgo = 0.0f;
        entry.entity = entity;
        entry.nodeId = nodeId;
        entry.nodeName = nodeName;
        entry.rawJson.clear();
        entry.status = status;

        m_executionLog.push_back(entry);

        while (m_executionLog.size() > MAX_LOG_ENTRIES)
        {
            m_executionLog.pop_front();
        }
        // Also record lightweight execution history for overlay (recent nodes)
        m_execHistory.emplace_back(entity, nodeId);
        while (m_execHistory.size() > MAX_EXEC_HISTORY)
        {
            m_execHistory.pop_front();
        }
    }

    // Helper to add to pending buffer when window isn't available
    static void AddToPendingBuffer(EntityID entity, uint32_t nodeId, const std::string& nodeName, BTStatus status)
    {
        ExecutionLogEntry entry;
        entry.timeAgo = 0.0f;
        entry.entity = entity;
        entry.nodeId = nodeId;
        entry.nodeName = nodeName;
        entry.status = status;

        s_pendingExecutionBuffer.push_back(entry);
        while (s_pendingExecutionBuffer.size() > MAX_PENDING_EXEC_BUFFER)
            s_pendingExecutionBuffer.pop_front();
    }

    // NodeGraph debug methods implemented in BehaviorTreeDebugWindow_NodeGraph.cpp
}

extern "C" {
    void BTDebug_AddExecutionEntry(EntityID entity, uint32_t nodeId, const char* nodeName, uint8_t status)
    {
        // Always accept entries. If the debug window is visible forward to it;
        // otherwise push into the module-level pending buffer so entries are
        // preserved while the UI is not available. This avoids calling into
        // window methods when the UI exists but is not ready (which can crash).
        if (!nodeName) return;

        // Prepare entry
        Olympe::ExecutionLogEntry e;
        e.timeAgo = 0.0f;
        e.entity = entity;
        e.nodeId = nodeId;
        e.nodeName = std::string(nodeName);
        e.status = static_cast<BTStatus>(status);

        if (g_btDebugWindow && g_btDebugWindow->IsVisible())
        {
            // Safe to forward directly to the visible window
            try { g_btDebugWindow->AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status); }
            catch (...) { /* swallow to avoid crashing runtime */ }
        }
        else
        {
            // Buffer for later flush when the window initializes or becomes visible
            Olympe::s_pendingExecutionBuffer.push_back(e);
            while (Olympe::s_pendingExecutionBuffer.size() > Olympe::MAX_PENDING_EXEC_BUFFER)
                Olympe::s_pendingExecutionBuffer.pop_front();
        }
    }

    void BTDebug_AddExecutionJson(const char* jsonLine)
    {
        static int s_debugPrints = 0;
        if (!jsonLine) return;
        // Lightweight arrival trace (helps diagnose calls that fail during parse)
        if (s_debugPrints < 5)
        {
            std::cout << "[BTDebug] Received JSON payload (raw): " << jsonLine << std::endl;
            ++s_debugPrints;
        }

        try
        {
            // Parse minimal fields to convert into internal ExecutionLogEntry
            auto j = nlohmann::json::parse(jsonLine);
            Olympe::ExecutionLogEntry e;
            e.timeAgo = 0.0f;
            // Parse numeric fields robustly: accept unsigned integers, signed numbers, or strings
            if (j.contains("entity"))
            {
                if (j["entity"].is_number()) e.entity = static_cast<EntityID>(static_cast<long long>(j["entity"].get<double>()));
                else if (j["entity"].is_string()) e.entity = static_cast<EntityID>(std::stoull(j["entity"].get<std::string>()));
                else e.entity = 0;
            }
            else e.entity = 0;

            if (j.contains("nodeId"))
            {
                if (j["nodeId"].is_number()) e.nodeId = static_cast<uint32_t>(static_cast<int>(j["nodeId"].get<double>()));
                else if (j["nodeId"].is_string()) e.nodeId = static_cast<uint32_t>(std::stoul(j["nodeId"].get<std::string>()));
                else e.nodeId = 0;
            }
            else e.nodeId = 0;

            e.nodeName = j.value("nodeName", std::string(""));
            e.rawJson = jsonLine;
            std::string statusStr = j.value("status", std::string("Running"));
            if (statusStr == "Success") e.status = BTStatus::Success;
            else if (statusStr == "Failure") e.status = BTStatus::Failure;
            else e.status = BTStatus::Running;

            // optional lightweight diagnostic output for first few calls
            if (s_debugPrints < 20)
            {
                std::cout << "[BTDebug] JSON entry: entity=" << e.entity << " nodeId=" << e.nodeId
                          << " nodeName=" << e.nodeName << " status=" << (int)e.status << std::endl;
                ++s_debugPrints;
            }

            // If window present and visible forward immediately, otherwise buffer
            if (g_btDebugWindow && g_btDebugWindow->IsVisible())
            {
                try { g_btDebugWindow->AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status); }
                catch (...) { /* swallow exceptions to avoid crashing runtime */ }
            }
            else
            {
                // Preserve parsed fields and keep original JSON for richer UI details later
                Olympe::s_pendingExecutionBuffer.push_back(e);
                while (Olympe::s_pendingExecutionBuffer.size() > Olympe::MAX_PENDING_EXEC_BUFFER)
                    Olympe::s_pendingExecutionBuffer.pop_front();
            }
        }
        catch (...)
        {
            // If parsing failed, still preserve the raw JSON so the UI can display
            // it when/if the debugger becomes available. Avoid dropping events.
            try
            {
                Olympe::ExecutionLogEntry e;
                e.timeAgo = 0.0f;
                e.entity = 0;
                e.nodeId = 0;
                e.nodeName = std::string("(unparsed)");
                e.rawJson = jsonLine;
                e.status = BTStatus::Running;

                if (g_btDebugWindow && g_btDebugWindow->IsVisible())
                {
                    try { g_btDebugWindow->AddExecutionEntry(e.entity, e.nodeId, e.nodeName, e.status); }
                    catch (...) { /* swallow */ }
                }
                else
                {
                    Olympe::s_pendingExecutionBuffer.push_back(e);
                    while (Olympe::s_pendingExecutionBuffer.size() > Olympe::MAX_PENDING_EXEC_BUFFER)
                        Olympe::s_pendingExecutionBuffer.pop_front();
                }
            }
            catch (...) { /* swallow double-failures */ }
        }
    }

    bool BTDebug_IsVisible()
    {
        return (g_btDebugWindow && g_btDebugWindow->IsVisible());
    }
}
