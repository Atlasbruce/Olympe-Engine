cmake_minimum_required(VERSION 3.14)
project(OlympeEngine CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

option(BUILD_TESTS                       "Build unit test executables"         OFF)
option(BUILD_RUNTIME_ENGINE              "Build the full runtime engine"        OFF)
option(BUILD_BLUEPRINT_EDITOR_STANDALONE "Build the standalone blueprint editor" OFF)

# ---------------------------------------------------------------------------
# Common include directories (used by tests)
# ---------------------------------------------------------------------------

set(SRC_DIR     "${CMAKE_SOURCE_DIR}/Source")
set(SDL_INC     "${CMAKE_SOURCE_DIR}/SDL/include")
set(SDL_INC_SDL3 "${CMAKE_SOURCE_DIR}/SDL/include/SDL3")

# All headers in Source/ are found via Source/ as the root.
# SDL headers (needed by vector.h → TaskGraphTypes.h) are under SDL/include and SDL/include/SDL3.
set(COMMON_INCLUDES
    "${SRC_DIR}"
    "${SDL_INC}"
    "${SDL_INC_SDL3}"
)

# ---------------------------------------------------------------------------
# Reusable source file groups
# ---------------------------------------------------------------------------

# Core TaskSystem sources (no SDL link required, only header access).
# NOTE: vector.cpp is excluded because it includes gameengine.h (SDL + engine deps).
#       The Vector class has an inline default ctor/copy ctor and all operators
#       used by tests are inline, so vector.cpp is not needed for unit tests.
set(TASK_SYSTEM_SOURCES
    "${SRC_DIR}/TaskSystem/TaskGraphTypes.cpp"
    "${SRC_DIR}/TaskSystem/TaskGraphTemplate.cpp"
    "${SRC_DIR}/TaskSystem/LocalBlackboard.cpp"
    "${SRC_DIR}/TaskSystem/AtomicTaskRegistry.cpp"
    "${SRC_DIR}/NodeGraphCore/GlobalBlackboard.cpp"
)

set(TASK_SYSTEM_FULL_SOURCES
    ${TASK_SYSTEM_SOURCES}
    "${SRC_DIR}/TaskSystem/TaskSystem.cpp"
    "${SRC_DIR}/TaskSystem/TaskExecutionBridge.cpp"
    "${SRC_DIR}/ECS/Components/TaskRunnerComponent.cpp"
    "${SRC_DIR}/ECS/Components/MovementComponent.cpp"
        "${SRC_DIR}/ECS/Components/PositionComponent.cpp"
)

set(TASK_GRAPH_LOADER_SOURCES
    "${SRC_DIR}/TaskSystem/TaskGraphLoader.cpp"
    "${SRC_DIR}/TaskSystem/TaskGraphMigrator_v3_to_v4.cpp"
    "${SRC_DIR}/BlueprintEditor/BTtoVSMigrator.cpp"
)

set(ASSET_MANAGER_SOURCES
    "${SRC_DIR}/Core/AssetManager.cpp"
)

set(VS_EXECUTOR_SOURCES
    "${SRC_DIR}/TaskSystem/VSGraphExecutor.cpp"
)

# TASK_SYSTEM_FULL_SOURCES + VS executor + Asset manager + Loader
# Required whenever TaskSystem.cpp is compiled (it references VSGraphExecutor + AssetManager)
set(TASK_SYSTEM_RUNTIME_SOURCES
    ${TASK_SYSTEM_FULL_SOURCES}
    ${VS_EXECUTOR_SOURCES}
    ${ASSET_MANAGER_SOURCES}
    ${TASK_GRAPH_LOADER_SOURCES}
)

# Phase 22-C registry sources (needed by VSGraphVerifier and editor panel)
set(PARAM_REGISTRY_SOURCES
    Source/BlueprintEditor/OperatorRegistry.cpp
    Source/BlueprintEditor/AtomicTaskUIRegistry.cpp
    Source/BlueprintEditor/ConditionRegistry.cpp
    Source/BlueprintEditor/BBVariableRegistry.cpp
    Source/BlueprintEditor/ParameterEditorRegistry.cpp
)

# Test stubs (provides SystemLogSink::AppendToLogWindow)
set(TEST_STUBS "${CMAKE_SOURCE_DIR}/Tests/TestStubs.cpp")

# ---------------------------------------------------------------------------
# Test targets
# ---------------------------------------------------------------------------

if(BUILD_TESTS)
    enable_testing()

    # -----------------------------------------------------------------------
    # OlympeDebuggerTests
    # -----------------------------------------------------------------------
    add_executable(OlympeDebuggerTests
        Tests/BlueprintEditor/DebuggerTest.cpp
        Source/BlueprintEditor/DebugController.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeDebuggerTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeDebuggerTests COMMAND OlympeDebuggerTests)

    # -----------------------------------------------------------------------
    # OlympeProfilerTests
    # -----------------------------------------------------------------------
    add_executable(OlympeProfilerTests
        Tests/BlueprintEditor/ProfilerTest.cpp
        Source/BlueprintEditor/PerformanceProfiler.cpp
        ${TEST_STUBS}
    )
    target_include_directories(OlympeProfilerTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeProfilerTests COMMAND OlympeProfilerTests)

    # -----------------------------------------------------------------------
    # OlympeVSEditorTests
    # -----------------------------------------------------------------------
    add_executable(OlympeVSEditorTests
        Tests/BlueprintEditor/VisualScriptEditorTest.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeVSEditorTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeVSEditorTests COMMAND OlympeVSEditorTests)

    # -----------------------------------------------------------------------
    # OlympeUndoRedoTests  (Phase 6)
    # -----------------------------------------------------------------------
    add_executable(OlympeUndoRedoTests
        Tests/BlueprintEditor/UndoRedoTest.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeUndoRedoTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeUndoRedoTests COMMAND OlympeUndoRedoTests)

    # -----------------------------------------------------------------------
    # OlympeAutoLayoutTests  (Phase 6)
    # -----------------------------------------------------------------------
    add_executable(OlympeAutoLayoutTests
        Tests/BlueprintEditor/AutoLayoutTest.cpp
        Source/BlueprintEditor/GraphAutoLayout.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeAutoLayoutTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeAutoLayoutTests COMMAND OlympeAutoLayoutTests)

    # -----------------------------------------------------------------------
    # OlympeBTtoVSMigratorTests  (Phase 6)
    # -----------------------------------------------------------------------
    add_executable(OlympeBTtoVSMigratorTests
        Tests/BlueprintEditor/BTtoVSMigratorTest.cpp
        Source/BlueprintEditor/BTtoVSMigrator.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeBTtoVSMigratorTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeBTtoVSMigratorTests COMMAND OlympeBTtoVSMigratorTests)

    # -----------------------------------------------------------------------
    # OlympeClipboardTests
    # -----------------------------------------------------------------------
    add_executable(OlympeClipboardTests
        Tests/Editor/ClipboardCopyPasteTest.cpp
        Source/BlueprintEditor/BTNodeGraphManager.cpp
        Source/BlueprintEditor/BPCommandSystem.cpp
        Source/BlueprintEditor/SubgraphMigrator.cpp
        ${TEST_STUBS}
    )
    target_include_directories(OlympeClipboardTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeClipboardTests COMMAND OlympeClipboardTests)

    # -----------------------------------------------------------------------
    # OlympeEditorRuntimeBridgeTests
    # -----------------------------------------------------------------------
    add_executable(OlympeEditorRuntimeBridgeTests
        Tests/Editor/EditorRuntimeBridgeTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeEditorRuntimeBridgeTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeEditorRuntimeBridgeTests COMMAND OlympeEditorRuntimeBridgeTests)

    # -----------------------------------------------------------------------
    # OlympeTaskSystemTests
    # -----------------------------------------------------------------------
    add_executable(OlympeTaskSystemTests
        Tests/TaskSystem/TaskSystemTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeTaskSystemTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeTaskSystemTests COMMAND OlympeTaskSystemTests)

    # -----------------------------------------------------------------------
    # OlympeTaskSystemRunningTests
    # -----------------------------------------------------------------------
    add_executable(OlympeTaskSystemRunningTests
        Tests/TaskSystem/TaskSystemRunningTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeTaskSystemRunningTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeTaskSystemRunningTests COMMAND OlympeTaskSystemRunningTests)

    # -----------------------------------------------------------------------
    # OlympeTaskSystemE2ETests
    # -----------------------------------------------------------------------
    add_executable(OlympeTaskSystemE2ETests
        Tests/TaskSystem/TaskSystem_E2E_ExampleGraph.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        "${SRC_DIR}/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp"
        "${SRC_DIR}/TaskSystem/AtomicTasks/Utility/Task_Wait.cpp"
        "${SRC_DIR}/TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeTaskSystemE2ETests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeTaskSystemE2ETests COMMAND OlympeTaskSystemE2ETests)

    # -----------------------------------------------------------------------
    # OlympeAtomicTaskRegistryTests
    # -----------------------------------------------------------------------
    add_executable(OlympeAtomicTaskRegistryTests
        Tests/TaskSystem/AtomicTaskRegistryTest.cpp
        "${SRC_DIR}/TaskSystem/AtomicTaskRegistry.cpp"
        "${SRC_DIR}/TaskSystem/TaskGraphTypes.cpp"
        "${SRC_DIR}/NodeGraphCore/GlobalBlackboard.cpp"
        "${SRC_DIR}/TaskSystem/AtomicTasks/Task_LogMessage.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeAtomicTaskRegistryTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeAtomicTaskRegistryTests COMMAND OlympeAtomicTaskRegistryTests)

    # -----------------------------------------------------------------------
    # OlympeLocalBlackboardTests
    # -----------------------------------------------------------------------
    add_executable(OlympeLocalBlackboardTests
        Tests/TaskSystem/LocalBlackboardTest.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeLocalBlackboardTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeLocalBlackboardTests COMMAND OlympeLocalBlackboardTests)

    # -----------------------------------------------------------------------
    # OlympeLocalBlackboardPersistenceTests
    # -----------------------------------------------------------------------
    add_executable(OlympeLocalBlackboardPersistenceTests
        Tests/TaskSystem/LocalBlackboardPersistenceTest.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeLocalBlackboardPersistenceTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeLocalBlackboardPersistenceTests COMMAND OlympeLocalBlackboardPersistenceTests)

    # -----------------------------------------------------------------------
    # OlympeTaskGraphLoaderTests
    # -----------------------------------------------------------------------
    add_executable(OlympeTaskGraphLoaderTests
        Tests/TaskSystem/TaskGraphLoaderTest.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeTaskGraphLoaderTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeTaskGraphLoaderTests COMMAND OlympeTaskGraphLoaderTests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # -----------------------------------------------------------------------
    # OlympeAssetManagerTaskGraphTests
    # -----------------------------------------------------------------------
    add_executable(OlympeAssetManagerTaskGraphTests
        Tests/TaskSystem/AssetManagerTaskGraphTest.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${ASSET_MANAGER_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeAssetManagerTaskGraphTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeAssetManagerTaskGraphTests COMMAND OlympeAssetManagerTaskGraphTests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # -----------------------------------------------------------------------
    # OlympeVSGraphExecutorTests
    # -----------------------------------------------------------------------
    add_executable(OlympeVSGraphExecutorTests
        Tests/TaskSystem/VSGraphExecutorTest.cpp
        ${TASK_SYSTEM_FULL_SOURCES}
        ${VS_EXECUTOR_SOURCES}
        ${ASSET_MANAGER_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeVSGraphExecutorTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeVSGraphExecutorTests COMMAND OlympeVSGraphExecutorTests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # -----------------------------------------------------------------------
    # OlympeSubGraphTests
    # -----------------------------------------------------------------------
    add_executable(OlympeSubGraphTests
        Tests/TaskSystem/SubGraphCompleteTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeSubGraphTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeSubGraphTests COMMAND OlympeSubGraphTests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # -----------------------------------------------------------------------
    # OlympeMoveToLocationLocalBBTests
    # -----------------------------------------------------------------------
    add_executable(OlympeMoveToLocationLocalBBTests
        Tests/TaskSystem/Task_MoveToLocation_LocalBBTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        "${SRC_DIR}/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeMoveToLocationLocalBBTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeMoveToLocationLocalBBTests COMMAND OlympeMoveToLocationLocalBBTests)

    # -----------------------------------------------------------------------
    # OlympeMoveToLocationWorldModeTests
    # -----------------------------------------------------------------------
    add_executable(OlympeMoveToLocationWorldModeTests
        Tests/TaskSystem/Task_MoveToLocation_WorldMode_Test.cpp
        "${SRC_DIR}/TaskSystem/AtomicTaskRegistry.cpp"
        "${SRC_DIR}/TaskSystem/TaskGraphTypes.cpp"
        "${SRC_DIR}/TaskSystem/TaskGraphTemplate.cpp"
        "${SRC_DIR}/TaskSystem/LocalBlackboard.cpp"
        "${SRC_DIR}/NodeGraphCore/GlobalBlackboard.cpp"
        "${SRC_DIR}/ECS/Components/MovementComponent.cpp"
        "${SRC_DIR}/ECS/Components/PositionComponent.cpp"
        "${SRC_DIR}/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeMoveToLocationWorldModeTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeMoveToLocationWorldModeTests COMMAND OlympeMoveToLocationWorldModeTests)

    # -----------------------------------------------------------------------
    # OlympeRequestPathfindingTests
    # -----------------------------------------------------------------------
    add_executable(OlympeRequestPathfindingTests
        Tests/TaskSystem/Task_RequestPathfindingTest.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        "${SRC_DIR}/TaskSystem/AtomicTasks/AI/Task_RequestPathfinding.cpp"
        "${SRC_DIR}/TaskSystem/Pathfinding/PathfindingManager.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeRequestPathfindingTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeRequestPathfindingTests COMMAND OlympeRequestPathfindingTests)

    # -----------------------------------------------------------------------
    # OlympeSetVariableTests
    # -----------------------------------------------------------------------
    add_executable(OlympeSetVariableTests
        Tests/TaskSystem/Task_SetVariable_Test.cpp
        "${SRC_DIR}/TaskSystem/AtomicTaskRegistry.cpp"
        "${SRC_DIR}/TaskSystem/TaskGraphTypes.cpp"
        "${SRC_DIR}/TaskSystem/TaskGraphTemplate.cpp"
        "${SRC_DIR}/TaskSystem/LocalBlackboard.cpp"
        "${SRC_DIR}/NodeGraphCore/GlobalBlackboard.cpp"
        "${SRC_DIR}/TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.cpp"
        ${TEST_STUBS}
    )
    target_include_directories(OlympeSetVariableTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeSetVariableTests COMMAND OlympeSetVariableTests)

    # -----------------------------------------------------------------------
    # OlympePhase7Tests  (Phase 7)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase7Tests
        Tests/BlueprintEditor/Phase7Test.cpp
        Source/BlueprintEditor/NodeSearchPalette.cpp
        Source/BlueprintEditor/MinimapWidget.cpp
        Source/BlueprintEditor/GridSnapping.cpp
        Source/BlueprintEditor/FileWatcher.cpp
        Source/BlueprintEditor/GraphValidationPanel.cpp
        Source/BlueprintEditor/Optimizations/ViewportCulling.cpp
        Source/BlueprintEditor/Optimizations/ConnectionCache.cpp
        Source/BlueprintEditor/Optimizations/AsyncJSONLoader.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympePhase7Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase7Tests COMMAND OlympePhase7Tests)

    # -----------------------------------------------------------------------
    # OlympePhase8Tests  (Phase 8 — SubgraphMigrator)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase8Tests
        Tests/BlueprintEditor/Phase8Test.cpp
        Source/BlueprintEditor/SubgraphMigrator.cpp
        ${TEST_STUBS}
    )
    target_include_directories(OlympePhase8Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase8Tests COMMAND OlympePhase8Tests)

    # -----------------------------------------------------------------------
    # OlympePhase9Tests  (Phase 9 — Advanced Node Features)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase9Tests
        Tests/BlueprintEditor/Phase9Test.cpp
        Source/NodeGraphCore/GraphComment.cpp
        Source/NodeGraphCore/NodeGroup.cpp
        Source/NodeGraphCore/NodeValidator.cpp
        Source/NodeGraphCore/LinkCache.cpp
        Source/BlueprintEditor/NodeSearchPanel.cpp
        Source/BlueprintEditor/MiniMapPanel.cpp
        Source/BlueprintEditor/NodePresets.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympePhase9Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase9Tests COMMAND OlympePhase9Tests)

    # -----------------------------------------------------------------------
    # OlympePhase10Tests  (Phase 10 — Blueprint Editor Tab System Fixes)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase10Tests
        Tests/BlueprintEditor/Phase10Test.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympePhase10Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase10Tests COMMAND OlympePhase10Tests)

    # -----------------------------------------------------------------------
    # OlympePhase11Tests  (Phase 11 — VS Editor Save Fixes)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase11Tests
        Tests/BlueprintEditor/Phase11Test.cpp
    )
    target_include_directories(OlympePhase11Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase11Tests COMMAND OlympePhase11Tests)

    # -----------------------------------------------------------------------
    # OlympePhase12Tests  (Phase 12 — Delete Nodes/Links & Context Menu Fixes)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase12Tests
        Tests/BlueprintEditor/Phase12Test.cpp
    )
    target_include_directories(OlympePhase12Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase12Tests COMMAND OlympePhase12Tests)

    # -----------------------------------------------------------------------
    # OlympePhase13Tests  (Phase 13 — Fix Undo/Redo After File Load)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase13Tests
        Tests/BlueprintEditor/Phase13Test.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase13Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase13Tests COMMAND OlympePhase13Tests)

    # -----------------------------------------------------------------------
    # OlympePhase14Tests  (Phase 14 — Fix Undo/Redo Completeness)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase14Tests
        Tests/BlueprintEditor/Phase14Test.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase14Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase14Tests COMMAND OlympePhase14Tests)

    # -----------------------------------------------------------------------
    # OlympePhase15Tests  (Phase 15 — Complete Undo/Redo + Context Menus)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase15Tests
        Tests/BlueprintEditor/Phase15Test.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase15Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase15Tests COMMAND OlympePhase15Tests)

    # -----------------------------------------------------------------------
    # OlympePhase20Tests  (Phase 20-B — Real-Time Connection Validation + Phase 20-C — Dynamic Pins)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase20Tests
        Tests/BlueprintEditor/Phase20Test.cpp
        Source/BlueprintEditor/VSConnectionValidator.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase20Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase20Tests COMMAND OlympePhase20Tests)

    # -----------------------------------------------------------------------
    # OlympePhase21ATests  (Phase 21-A — VSGraphVerifier global graph verifier)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase21ATests
        Tests/BlueprintEditor/Phase21ATest.cpp
        Source/BlueprintEditor/VSGraphVerifier.cpp
        ${PARAM_REGISTRY_SOURCES}
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase21ATests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase21ATests COMMAND OlympePhase21ATests)

    # -----------------------------------------------------------------------
    # OlympePhase21BTests  (Phase 21-B — VSGraphVerifier UI Integration)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase21BTests
        Tests/BlueprintEditor/Phase21BTest.cpp
        Source/BlueprintEditor/VSGraphVerifier.cpp
        ${PARAM_REGISTRY_SOURCES}
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase21BTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase21BTests COMMAND OlympePhase21BTests)

    # -----------------------------------------------------------------------
    # OlympePhase21DTests  (Phase 21-D — Dynamic Pins Sequence/Switch)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase21DTests
        Tests/BlueprintEditor/Phase21DTest.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase21DTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase21DTests COMMAND OlympePhase21DTests)

    # -----------------------------------------------------------------------
    # OlympeATSReferenceV4Tests  (Integration — reference_complete_v4.ats)
    # -----------------------------------------------------------------------
    add_executable(OlympeATSReferenceV4Tests
        Tests/Integration/ATSReferenceV4Test.cpp
        ${TASK_SYSTEM_RUNTIME_SOURCES}
        ${TEST_STUBS}
    )
    target_include_directories(OlympeATSReferenceV4Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympeATSReferenceV4Tests COMMAND OlympeATSReferenceV4Tests
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # -----------------------------------------------------------------------
    # OlympePhase22CTests  (Phase 22-C — Parameter Dropdowns & Registries)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase22CTests
        Tests/BlueprintEditor/Phase22CTest.cpp
        Source/BlueprintEditor/OperatorRegistry.cpp
        Source/BlueprintEditor/AtomicTaskUIRegistry.cpp
        Source/BlueprintEditor/ConditionRegistry.cpp
        Source/BlueprintEditor/BBVariableRegistry.cpp
        Source/BlueprintEditor/ParameterEditorRegistry.cpp
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase22CTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase22CTests COMMAND OlympePhase22CTests)

    # -----------------------------------------------------------------------
    # OlympePhase23BTests  (Phase 23-B — Full Blackboard Properties + BUG-001)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase23BTests
        Tests/BlueprintEditor/Phase23BTest.cpp
        Source/BlueprintEditor/BlackboardVariablePresetRegistry.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase23BTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase23BTests COMMAND OlympePhase23BTests)

    # -----------------------------------------------------------------------
    # OlympePhase23B2Tests  (Phase 23-B.2 — HOTFIX BUG-003 & BUG-004: Node
    #                         Position Offset + Load Crash)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase23B2Tests
        Tests/BlueprintEditor/Phase23B2Test.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase23B2Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase23B2Tests COMMAND OlympePhase23B2Tests)

    # -----------------------------------------------------------------------
    # OlympePhase23B4Tests  (Phase 23-B.4 — Unified Condition System)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase23B4Tests
        Tests/BlueprintEditor/Phase23B4Test.cpp
        Source/TaskSystem/ConditionEvaluator.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        Source/BlueprintEditor/VSGraphVerifier.cpp
        ${PARAM_REGISTRY_SOURCES}
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase23B4Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase23B4Tests COMMAND OlympePhase23B4Tests)

    # OlympePhase23B5Tests  (Phase 23-B.5 — BUG-028 Float Serialization & BUG-029 Preview)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase23B5Tests
        Tests/BlueprintEditor/Phase23B5Test.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TASK_GRAPH_LOADER_SOURCES}
        ${PARAM_REGISTRY_SOURCES}
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase23B5Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase23B5Tests COMMAND OlympePhase23B5Tests)

    # OlympePhase23B6Tests  (Phase 23-B.6 — BUG-030 ImGui Conflicting ID in Condition Builder)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase23B6Tests
        Tests/BlueprintEditor/Phase23B6Test.cpp
        ${TASK_SYSTEM_SOURCES}
        ${PARAM_REGISTRY_SOURCES}
        Tests/TestStubs.cpp
    )
    target_include_directories(OlympePhase23B6Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase23B6Tests COMMAND OlympePhase23B6Tests)

    # -----------------------------------------------------------------------
    # OlympePhase24_0_OperandTests  (Phase 24.0 — Operand)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_0_OperandTests
        Tests/Editor/ConditionPreset/OperandTest.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_0_OperandTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_0_OperandTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_0_OperandTests COMMAND OlympePhase24_0_OperandTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_0_ConditionPresetTests  (Phase 24.0 — ConditionPreset)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_0_ConditionPresetTests
        Tests/Editor/ConditionPreset/ConditionPresetTest.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_0_ConditionPresetTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_0_ConditionPresetTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_0_ConditionPresetTests COMMAND OlympePhase24_0_ConditionPresetTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_0_RegistryTests  (Phase 24.0 — ConditionPresetRegistry)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_0_RegistryTests
        Tests/Editor/ConditionPreset/ConditionPresetRegistryTest.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_0_RegistryTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_0_RegistryTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_0_RegistryTests COMMAND OlympePhase24_0_RegistryTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_0_DynamicDataPinTests  (Phase 24.0 — DynamicDataPin)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_0_DynamicDataPinTests
        Tests/Editor/ConditionPreset/DynamicDataPinTest.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_0_DynamicDataPinTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_0_DynamicDataPinTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_0_DynamicDataPinTests COMMAND OlympePhase24_0_DynamicDataPinTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_1_LibraryPanelTests  (Phase 24.1 — ConditionPresetLibraryPanel)
    # NOTE: Disabled — tests use the OLD ConditionPreset API (Condition sub-struct)
    #       and reference Source/BlueprintEditor/ConditionPreset.cpp which was
    #       removed when Phase 24.0 migrated to the Operand-based model.
    #       These tests need a full rewrite against the Phase 24.0 API.
    # -----------------------------------------------------------------------
    # add_executable(OlympePhase24_1_LibraryPanelTests
    #     Tests/Editor/Panels/ConditionPresetLibraryPanelTest.cpp
    #     Source/BlueprintEditor/ConditionPreset.cpp
    #     Source/Editor/Panels/ConditionPresetLibraryPanel.cpp
    #     Source/TaskSystem/TaskGraphTypes.cpp
    #     Source/TaskSystem/TaskGraphTemplate.cpp
    #     Source/TaskSystem/LocalBlackboard.cpp
    #     Source/TaskSystem/AtomicTaskRegistry.cpp
    #     Source/NodeGraphCore/GlobalBlackboard.cpp
    #     Tests/TestStubs.cpp
    # )
    # target_compile_definitions(OlympePhase24_1_LibraryPanelTests PRIVATE OLYMPE_HEADLESS)
    # target_include_directories(OlympePhase24_1_LibraryPanelTests PRIVATE ${COMMON_INCLUDES})
    # add_test(NAME OlympePhase24_1_LibraryPanelTests COMMAND OlympePhase24_1_LibraryPanelTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_1_EditDialogTests  (Phase 24.1 — ConditionPresetEditDialog)
    # NOTE: Disabled — same reason as OlympePhase24_1_LibraryPanelTests above.
    # -----------------------------------------------------------------------
    # add_executable(OlympePhase24_1_EditDialogTests
    #     Tests/Editor/Dialogs/ConditionPresetEditDialogTest.cpp
    #     Source/BlueprintEditor/ConditionPreset.cpp
    #     Source/Editor/Dialogs/ConditionPresetEditDialog.cpp
    #     Source/TaskSystem/TaskGraphTypes.cpp
    #     Source/TaskSystem/TaskGraphTemplate.cpp
    #     Source/TaskSystem/LocalBlackboard.cpp
    #     Source/TaskSystem/AtomicTaskRegistry.cpp
    #     Source/NodeGraphCore/GlobalBlackboard.cpp
    #     Tests/TestStubs.cpp
    # )
    # target_compile_definitions(OlympePhase24_1_EditDialogTests PRIVATE OLYMPE_HEADLESS)
    # target_include_directories(OlympePhase24_1_EditDialogTests PRIVATE ${COMMON_INCLUDES})
    # add_test(NAME OlympePhase24_1_EditDialogTests COMMAND OlympePhase24_1_EditDialogTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_2_NodeConditionsPanelTests  (Phase 24.2 — NodeConditionsPanel)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_2_NodeConditionsPanelTests
        Tests/Editor/Panels/NodeConditionsPanelTest.cpp
        Source/Editor/Panels/NodeConditionsPanel.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_2_NodeConditionsPanelTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_2_NodeConditionsPanelTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_2_NodeConditionsPanelTests COMMAND OlympePhase24_2_NodeConditionsPanelTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_3_DynamicDataPinManagerTests  (Phase 24.3 — DynamicDataPinManager)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_3_DynamicDataPinManagerTests
        Tests/Editor/ConditionPreset/DynamicDataPinManagerTest.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_3_DynamicDataPinManagerTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_3_DynamicDataPinManagerTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_3_DynamicDataPinManagerTests COMMAND OlympePhase24_3_DynamicDataPinManagerTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_4_NodeBranchRendererTests  (Phase 24.4 — NodeBranchRenderer)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_4_NodeBranchRendererTests
        Tests/Editor/Nodes/NodeBranchRendererTest.cpp
        Source/Editor/Nodes/NodeBranchRenderer.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_4_NodeBranchRendererTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_4_NodeBranchRendererTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_4_NodeBranchRendererTests COMMAND OlympePhase24_4_NodeBranchRendererTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_REFONTE_EditModalTests  (Phase 24-REFONTE — NodeConditionsEditModal)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_REFONTE_EditModalTests
        Tests/Editor/Modals/NodeConditionsEditModalTest.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_REFONTE_EditModalTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_REFONTE_EditModalTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_REFONTE_EditModalTests COMMAND OlympePhase24_REFONTE_EditModalTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_REFONTE_DynamicPinTests  (Phase 24-REFONTE — Dynamic Pin Integration)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_REFONTE_DynamicPinTests
        Tests/Phase24/Phase24DynamicPinTest.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/Panels/NodeConditionsPanel.cpp
        Source/Editor/Nodes/NodeBranchRenderer.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_REFONTE_DynamicPinTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_REFONTE_DynamicPinTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_REFONTE_DynamicPinTests COMMAND OlympePhase24_REFONTE_DynamicPinTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_Rendering_Tests  (Phase 24-Rendering — Visual Refactoring)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_Rendering_Tests
        Tests/Phase24Rendering/Phase24RenderingTest.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/Panels/NodeConditionsPanel.cpp
        Source/Editor/Nodes/NodeBranchRenderer.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_Rendering_Tests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_Rendering_Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_Rendering_Tests COMMAND OlympePhase24_Rendering_Tests)

    # -----------------------------------------------------------------------
    # OlympePhase24_Integration_Tests  (Phase 24-Rendering Integration — PR #444)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_Integration_Tests
        Tests/Phase24Integration/Phase24IntegrationTest.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/Panels/NodeConditionsPanel.cpp
        Source/Editor/Nodes/NodeBranchRenderer.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Tests/TestStubs.cpp
    )
    target_compile_definitions(OlympePhase24_Integration_Tests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_Integration_Tests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_Integration_Tests COMMAND OlympePhase24_Integration_Tests)

    # -----------------------------------------------------------------------
    # OlympePhase24_5_RuntimeTests  (Phase 24.5 — Runtime Condition Evaluation)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_5_RuntimeTests
        Tests/Runtime/Phase24RuntimeTest.cpp
        Source/Runtime/RuntimeEnvironment.cpp
        Source/Runtime/ConditionPresetEvaluator.cpp
        Source/Runtime/GraphRuntimeInstance.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        ${TASK_SYSTEM_SOURCES}
        ${TEST_STUBS}
    )
    target_compile_definitions(OlympePhase24_5_RuntimeTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_5_RuntimeTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_5_RuntimeTests COMMAND OlympePhase24_5_RuntimeTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_M22_ConditionRefTests  (Phase 24 M2.2 — ConditionRef/OperandRef)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_M22_ConditionRefTests
        Tests/Editor/ConditionRef_Tests.cpp
        Source/BlueprintEditor/ConditionRef.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        ${TEST_STUBS}
    )
    target_compile_definitions(OlympePhase24_M22_ConditionRefTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_M22_ConditionRefTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_M22_ConditionRefTests COMMAND OlympePhase24_M22_ConditionRefTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_M22_DynPinManagerTests  (Phase 24 M2.2 — DynamicDataPinManager pin naming)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_M22_DynPinManagerTests
        Tests/Editor/DynamicDataPinManager_Tests.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        ${TEST_STUBS}
    )
    target_compile_definitions(OlympePhase24_M22_DynPinManagerTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_M22_DynPinManagerTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_M22_DynPinManagerTests COMMAND OlympePhase24_M22_DynPinManagerTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_M22_ConditionSerializationTests  (Phase 24 M2.2 — JSON round-trip)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_M22_ConditionSerializationTests
        Tests/Editor/ConditionSerialization_Tests.cpp
        Source/BlueprintEditor/ConditionRef.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        ${TEST_STUBS}
    )
    target_compile_definitions(OlympePhase24_M22_ConditionSerializationTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_M22_ConditionSerializationTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_M22_ConditionSerializationTests COMMAND OlympePhase24_M22_ConditionSerializationTests)

    # -----------------------------------------------------------------------
    # OlympePhase24_M22_FullRoundTripTests  (Phase 24 M2.2.2 — Full Stack Round-Trip)
    # -----------------------------------------------------------------------
    add_executable(OlympePhase24_M22_FullRoundTripTests
        Tests/System/Phase24_FullRoundTrip_Tests.cpp
        Source/BlueprintEditor/ConditionRef.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        ${TEST_STUBS}
    )
    target_compile_definitions(OlympePhase24_M22_FullRoundTripTests PRIVATE OLYMPE_HEADLESS)
    target_include_directories(OlympePhase24_M22_FullRoundTripTests PRIVATE ${COMMON_INCLUDES})
    add_test(NAME OlympePhase24_M22_FullRoundTripTests COMMAND OlympePhase24_M22_FullRoundTripTests)

endif() # BUILD_TESTS
