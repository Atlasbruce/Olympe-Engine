cmake_minimum_required(VERSION 3.14)
project(OlympeEngine CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(BUILD_TESTS                        "Build unit tests"                    OFF)
option(BUILD_RUNTIME_ENGINE               "Build the runtime engine"             ON)
option(BUILD_BLUEPRINT_EDITOR_STANDALONE  "Build the blueprint editor standalone" ON)

# ---------------------------------------------------------------------------
# Test build
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()

    # -----------------------------------------------------------------------
    # Common include paths for all test targets
    # Priority:
    #   1. Tests/sdl3_stub  – minimal SDL3 types so headers compile headless
    #   2. Source/          – engine headers (relative includes from Source/)
    #   3. Source/third_party – nlohmann/json, imgui, imnodes
    # -----------------------------------------------------------------------
    set(TEST_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/Tests/sdl3_stub
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party
        ${CMAKE_SOURCE_DIR}/Source/third_party/imgui
        ${CMAKE_SOURCE_DIR}/Source/third_party/imnodes
    )

    set(TEST_COMPILE_DEFS OLYMPE_HEADLESS)

    # -----------------------------------------------------------------------
    # engine_core static library
    # Contains all non-UI source files that tests link against.
    # OLYMPE_HEADLESS strips any remaining ImGui/ImNodes calls via #ifdef.
    # -----------------------------------------------------------------------
    set(ENGINE_CORE_SOURCES
        # TaskSystem
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphMigrator_v3_to_v4.cpp
        Source/TaskSystem/ConditionEvaluator.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/EntityBlackboard.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/VSGraphExecutor.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/TaskExecutionBridge.cpp
        Source/TaskSystem/Pathfinding/PathfindingManager.cpp
        # AtomicTasks
        Source/TaskSystem/AtomicTasks/Task_LogMessage.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_ChangeState.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_Flee.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_GotoPosition.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_RequestPathfinding.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_SetAnimation.cpp
        Source/TaskSystem/AtomicTasks/Blackboard/Task_Compare.cpp
        Source/TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.cpp
        Source/TaskSystem/AtomicTasks/Combat/Task_Attack.cpp
        Source/TaskSystem/AtomicTasks/Combat/Task_Flee.cpp
        Source/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp
        Source/TaskSystem/AtomicTasks/Rendering/Task_PlayAnimation.cpp
        Source/TaskSystem/AtomicTasks/Rendering/Task_SetSprite.cpp
        Source/TaskSystem/AtomicTasks/State/Task_ChangeState.cpp
        Source/TaskSystem/AtomicTasks/System/Task_GetDeltaTime.cpp
        Source/TaskSystem/AtomicTasks/System/Task_GetWorldInstance.cpp
        Source/TaskSystem/AtomicTasks/System/Task_SetPosition.cpp
        Source/TaskSystem/AtomicTasks/Utility/Task_Wait.cpp
        # BlueprintEditor (non-UI)
        Source/BlueprintEditor/UndoRedoStack.cpp
        Source/BlueprintEditor/ConditionRef.cpp
        Source/BlueprintEditor/MathOpOperand.cpp
        Source/BlueprintEditor/BTtoVSMigrator.cpp
        Source/BlueprintEditor/VSGraphVerifier.cpp
        Source/BlueprintEditor/ConditionRegistry.cpp
        Source/BlueprintEditor/OperatorRegistry.cpp
        Source/BlueprintEditor/AtomicTaskUIRegistry.cpp
        Source/BlueprintEditor/GraphAutoLayout.cpp
        Source/BlueprintEditor/PerformanceProfiler.cpp
        Source/BlueprintEditor/VSConnectionValidator.cpp
        Source/BlueprintEditor/FileWatcher.cpp
        Source/BlueprintEditor/GraphValidationPanel.cpp
        Source/BlueprintEditor/GridSnapping.cpp
        Source/BlueprintEditor/NodeSearchPalette.cpp
        Source/BlueprintEditor/NodeSearchPanel.cpp
        Source/BlueprintEditor/MinimapWidget.cpp
        Source/BlueprintEditor/MiniMapPanel.cpp
        Source/BlueprintEditor/NodePresets.cpp
        Source/BlueprintEditor/BPCommandSystem.cpp
        Source/BlueprintEditor/BTNodeGraphManager.cpp
        Source/BlueprintEditor/BBVariableRegistry.cpp
        Source/BlueprintEditor/BlackboardVariablePresetRegistry.cpp
        Source/BlueprintEditor/SubgraphMigrator.cpp
        Source/BlueprintEditor/ParameterEditorRegistry.cpp
        Source/BlueprintEditor/DebugController.cpp
        Source/BlueprintEditor/Optimizations/ViewportCulling.cpp
        Source/BlueprintEditor/Optimizations/ConnectionCache.cpp
        Source/BlueprintEditor/VisualScriptEditorPanel_Interaction.cpp
        Source/BlueprintEditor/VisualScriptEditorPanel_Presets.cpp
        # ECS components (needed by Task tests)
        Source/ECS/Components/MovementComponent.cpp
        Source/ECS/Components/PositionComponent.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        # NodeGraphCore extras
        Source/NodeGraphCore/GraphComment.cpp
        Source/NodeGraphCore/LinkCache.cpp
        Source/NodeGraphCore/NodeGroup.cpp
        Source/NodeGraphCore/NodeValidator.cpp
        # Editor / ConditionPreset
        Source/Editor/ConditionPreset/Operand.cpp
        Source/Editor/ConditionPreset/ConditionPreset.cpp
        Source/Editor/ConditionPreset/ConditionPresetRegistry.cpp
        Source/Editor/ConditionPreset/DynamicDataPin.cpp
        Source/Editor/ConditionPreset/DynamicDataPinManager.cpp
        Source/Editor/ConditionPreset/NodeConditionRef.cpp
        # Editor UI panels (headless: ImGui calls stripped by OLYMPE_HEADLESS)
        Source/Editor/Nodes/NodeBranchRenderer.cpp
        Source/Editor/Panels/NodeConditionsPanel.cpp
        Source/Editor/Modals/NodeConditionsEditModal.cpp
        Source/Editor/Panels/ConditionPresetLibraryPanel.cpp
        Source/Editor/Dialogs/ConditionPresetEditDialog.cpp
        Source/Editor/UIHelpers/PresetDropdownHelper.cpp
        # NodeGraphCore
        Source/NodeGraphCore/GlobalBlackboard.cpp
        Source/NodeGraphCore/GlobalTemplateBlackboard.cpp
        # Core
        Source/Core/AssetManager.cpp
        # Runtime
        Source/Runtime/RuntimeEnvironment.cpp
        Source/Runtime/GraphRuntimeInstance.cpp
        Source/Runtime/ConditionPresetEvaluator.cpp
        # Test stubs (SystemLogSink::AppendToLogWindow etc.)
        Tests/TestStubs.cpp
    )

    add_library(engine_core STATIC ${ENGINE_CORE_SOURCES})
    target_include_directories(engine_core PUBLIC ${TEST_INCLUDE_DIRS})
    target_compile_definitions(engine_core PUBLIC ${TEST_COMPILE_DEFS})

    # -----------------------------------------------------------------------
    # Macro to add a test executable
    # Usage: add_olympe_test(target test_file [extra_src...])
    # -----------------------------------------------------------------------
    macro(add_olympe_test TARGET TEST_FILE)
        set(_extra_srcs ${ARGN})
        add_executable(${TARGET} ${TEST_FILE} ${_extra_srcs})
        target_include_directories(${TARGET} PRIVATE ${TEST_INCLUDE_DIRS})
        target_compile_definitions(${TARGET} PRIVATE ${TEST_COMPILE_DEFS})
        target_link_libraries(${TARGET} PRIVATE engine_core)
        add_test(NAME ${TARGET} COMMAND ${TARGET})
    endmacro()

    # -----------------------------------------------------------------------
    # BlueprintEditor tests
    # -----------------------------------------------------------------------
    add_olympe_test(UndoRedoTest
        Tests/BlueprintEditor/UndoRedoTest.cpp)

    add_olympe_test(AutoLayoutTest
        Tests/BlueprintEditor/AutoLayoutTest.cpp)

    add_olympe_test(BTtoVSMigratorTest
        Tests/BlueprintEditor/BTtoVSMigratorTest.cpp)

    add_olympe_test(DebuggerTest
        Tests/BlueprintEditor/DebuggerTest.cpp)

    add_olympe_test(ProfilerTest
        Tests/BlueprintEditor/ProfilerTest.cpp)

    add_olympe_test(Phase7Test
        Tests/BlueprintEditor/Phase7Test.cpp)

    add_olympe_test(Phase8Test
        Tests/BlueprintEditor/Phase8Test.cpp)

    add_olympe_test(Phase9Test
        Tests/BlueprintEditor/Phase9Test.cpp)

    add_olympe_test(Phase10Test
        Tests/BlueprintEditor/Phase10Test.cpp)

    add_olympe_test(Phase11Test
        Tests/BlueprintEditor/Phase11Test.cpp)

    add_olympe_test(Phase12Test
        Tests/BlueprintEditor/Phase12Test.cpp)

    add_olympe_test(Phase13Test
        Tests/BlueprintEditor/Phase13Test.cpp)

    add_olympe_test(Phase14Test
        Tests/BlueprintEditor/Phase14Test.cpp)

    add_olympe_test(Phase15Test
        Tests/BlueprintEditor/Phase15Test.cpp)

    add_olympe_test(Phase20Test
        Tests/BlueprintEditor/Phase20Test.cpp)

    add_olympe_test(Phase21ATest
        Tests/BlueprintEditor/Phase21ATest.cpp)

    add_olympe_test(Phase21BTest
        Tests/BlueprintEditor/Phase21BTest.cpp)

    add_olympe_test(Phase21DTest
        Tests/BlueprintEditor/Phase21DTest.cpp)

    add_olympe_test(Phase22CTest
        Tests/BlueprintEditor/Phase22CTest.cpp)

    add_olympe_test(Phase23BTest
        Tests/BlueprintEditor/Phase23BTest.cpp)

    add_olympe_test(Phase23B2Test
        Tests/BlueprintEditor/Phase23B2Test.cpp)

    add_olympe_test(Phase23B4Test
        Tests/BlueprintEditor/Phase23B4Test.cpp)

    add_olympe_test(Phase23B5Test
        Tests/BlueprintEditor/Phase23B5Test.cpp)

    add_olympe_test(Phase23B6Test
        Tests/BlueprintEditor/Phase23B6Test.cpp)

    add_olympe_test(VisualScriptEditorTest
        Tests/BlueprintEditor/VisualScriptEditorTest.cpp)

    # -----------------------------------------------------------------------
    # Editor / ConditionPreset tests
    # -----------------------------------------------------------------------
    add_olympe_test(OperandTest
        Tests/Editor/ConditionPreset/OperandTest.cpp)

    add_olympe_test(ConditionPresetTest
        Tests/Editor/ConditionPreset/ConditionPresetTest.cpp)

    add_olympe_test(ConditionPresetRegistryTest
        Tests/Editor/ConditionPreset/ConditionPresetRegistryTest.cpp)

    add_olympe_test(DynamicDataPinTest
        Tests/Editor/ConditionPreset/DynamicDataPinTest.cpp)

    add_olympe_test(DynamicDataPinManagerTest
        Tests/Editor/ConditionPreset/DynamicDataPinManagerTest.cpp)

    add_olympe_test(ConditionRefTests
        Tests/Editor/ConditionRef_Tests.cpp)

    add_olympe_test(ConditionSerializationTests
        Tests/Editor/ConditionSerialization_Tests.cpp)

    add_olympe_test(DynamicDataPinManagerTests
        Tests/Editor/DynamicDataPinManager_Tests.cpp)

    add_olympe_test(NodeBranchRendererTest
        Tests/Editor/Nodes/NodeBranchRendererTest.cpp)

    add_olympe_test(NodeConditionsPanelTest
        Tests/Editor/Panels/NodeConditionsPanelTest.cpp)

    add_olympe_test(ConditionPresetLibraryPanelTest
        Tests/Editor/Panels/ConditionPresetLibraryPanelTest.cpp)

    add_olympe_test(NodeConditionsEditModalTest
        Tests/Editor/Modals/NodeConditionsEditModalTest.cpp)

    add_olympe_test(ConditionPresetEditDialogTest
        Tests/Editor/Dialogs/ConditionPresetEditDialogTest.cpp)

    add_olympe_test(ClipboardCopyPasteTest
        Tests/Editor/ClipboardCopyPasteTest.cpp)

    add_olympe_test(EditorRuntimeBridgeTest
        Tests/Editor/EditorRuntimeBridgeTest.cpp)

    # -----------------------------------------------------------------------
    # TaskSystem tests
    # -----------------------------------------------------------------------
    add_olympe_test(TaskGraphLoaderTest
        Tests/TaskSystem/TaskGraphLoaderTest.cpp)

    add_olympe_test(TaskSystemTest
        Tests/TaskSystem/TaskSystemTest.cpp)

    add_olympe_test(TaskSystemRunningTest
        Tests/TaskSystem/TaskSystemRunningTest.cpp)

    add_olympe_test(VSGraphExecutorTest
        Tests/TaskSystem/VSGraphExecutorTest.cpp)

    add_olympe_test(LocalBlackboardTest
        Tests/TaskSystem/LocalBlackboardTest.cpp)

    add_olympe_test(LocalBlackboardPersistenceTest
        Tests/TaskSystem/LocalBlackboardPersistenceTest.cpp)

    add_olympe_test(SubGraphCompleteTest
        Tests/TaskSystem/SubGraphCompleteTest.cpp)

    add_olympe_test(AtomicTaskRegistryTest
        Tests/TaskSystem/AtomicTaskRegistryTest.cpp)

    add_olympe_test(AssetManagerTaskGraphTest
        Tests/TaskSystem/AssetManagerTaskGraphTest.cpp)

    add_olympe_test(Task_SetVariable_Test
        Tests/TaskSystem/Task_SetVariable_Test.cpp)

    add_olympe_test(Task_MoveToLocation_LocalBBTest
        Tests/TaskSystem/Task_MoveToLocation_LocalBBTest.cpp)

    add_olympe_test(Task_MoveToLocation_WorldMode_Test
        Tests/TaskSystem/Task_MoveToLocation_WorldMode_Test.cpp)

    add_olympe_test(Task_RequestPathfindingTest
        Tests/TaskSystem/Task_RequestPathfindingTest.cpp)

    add_olympe_test(TaskSystem_E2E_ExampleGraph
        Tests/TaskSystem/TaskSystem_E2E_ExampleGraph.cpp)

    # -----------------------------------------------------------------------
    # NodeGraphCore tests
    # -----------------------------------------------------------------------
    add_olympe_test(GlobalTemplateBlackboardSerializationTest
        Tests/NodeGraphCore/GlobalTemplateBlackboardSerializationTest.cpp)

    # -----------------------------------------------------------------------
    # Phase 24 tests
    # -----------------------------------------------------------------------
    add_olympe_test(Phase24DynamicPinTest
        Tests/Phase24/Phase24DynamicPinTest.cpp)

    add_olympe_test(Phase24IntegrationTest
        Tests/Phase24Integration/Phase24IntegrationTest.cpp)

    add_olympe_test(Phase24RenderingTest
        Tests/Phase24Rendering/Phase24RenderingTest.cpp)

    # -----------------------------------------------------------------------
    # Runtime tests
    # -----------------------------------------------------------------------
    add_olympe_test(Phase24RuntimeTest
        Tests/Runtime/Phase24RuntimeTest.cpp)

    # -----------------------------------------------------------------------
    # System / integration tests
    # -----------------------------------------------------------------------
    add_olympe_test(Phase24_FullRoundTrip_Tests
        Tests/System/Phase24_FullRoundTrip_Tests.cpp)

    add_olympe_test(ATSReferenceV4Test
        Tests/Integration/ATSReferenceV4Test.cpp)

endif() # BUILD_TESTS
