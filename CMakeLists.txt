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

endif() # BUILD_TESTS
