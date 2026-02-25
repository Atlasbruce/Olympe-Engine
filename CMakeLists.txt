cmake_minimum_required(VERSION 3.14)
project(OlympeEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_RUNTIME_ENGINE "Build runtime engine executable" ON)
option(BUILD_BLUEPRINT_EDITOR_STANDALONE "Build Blueprint Editor Standalone executable" ON)
option(BUILD_TESTS "Build unit tests" OFF)

# SDL3 and the shared OlympeCore library are required only for engine targets.
# When BUILD_TESTS=ON with both engine options OFF the build works without SDL3.
if(BUILD_RUNTIME_ENGINE OR BUILD_BLUEPRINT_EDITOR_STANDALONE)

# Find SDL3
find_package(SDL3 REQUIRED)

# ============================================================================
# TiledLevelLoader - Tiled MapEditor file loader
# ============================================================================
add_subdirectory(Source/TiledLevelLoader)

# ============================================================================
# Shared Blueprint Editor Sources
# ============================================================================
file(GLOB_RECURSE BLUEPRINT_EDITOR_SOURCES "Source/BlueprintEditor/*.cpp")
file(GLOB_RECURSE IMGUI_SOURCES 
    "Source/third_party/imgui/*.cpp" 
    "Source/third_party/imgui/backends/imgui_impl_sdl3.cpp"
    "Source/third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp"
)
file(GLOB_RECURSE IMNODES_SOURCES "Source/third_party/imnodes/*.cpp")
file(GLOB_RECURSE NODEGRAPHCORE_SOURCES 
    "Source/NodeGraphCore/*.cpp"
    "Source/NodeGraphCore/Commands/*.cpp"
)
file(GLOB_RECURSE ENGINE_CORE_SOURCES 
    "Source/AI/*.cpp"
    "Source/system/*.cpp"
    "Source/DataManager.cpp"
    "Source/drawing.cpp"
    "Source/InputsManager.cpp"
    "Source/PrefabScanner.cpp"
)
file(GLOB_RECURSE TASKSYSTEM_SOURCES
    "Source/TaskSystem/*.cpp"
)

# ============================================================================
# OlympeCore - Shared Library for Blueprint Editor
# ============================================================================
add_library(OlympeCore STATIC
    ${BLUEPRINT_EDITOR_SOURCES}
    ${IMGUI_SOURCES}
    ${IMNODES_SOURCES}
    ${NODEGRAPHCORE_SOURCES}
    ${TASKSYSTEM_SOURCES}
    Source/ECS/Components/TaskRunnerComponent.cpp
    Source/AI/BehaviorTree.cpp
    Source/AI/BehaviorTreeDependencyScanner.cpp
    Source/DataManager.cpp
    Source/drawing.cpp
    Source/PrefabScanner.cpp
    Source/ComponentDefinition.cpp
    Source/third_party/nfd/nfd_win.cpp
)

target_include_directories(OlympeCore PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/third_party
    ${CMAKE_SOURCE_DIR}/Source/third_party/imgui
    ${CMAKE_SOURCE_DIR}/Source/third_party/imnodes
    ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/Source/third_party/nfd
    ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(OlympeCore PUBLIC SDL3::SDL3 TiledLevelLoader)

# Add NFD platform-specific linker flags
if(WIN32)
    target_link_libraries(OlympeCore PUBLIC Comctl32 Ole32 Shell32)
endif()

target_compile_definitions(OlympeCore PUBLIC OLYMPE_BLUEPRINT_EDITOR_ENABLED)

# C++14 standard
set_target_properties(OlympeCore PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

endif() # BUILD_RUNTIME_ENGINE OR BUILD_BLUEPRINT_EDITOR_STANDALONE

# ============================================================================
# Exécutable 1: Runtime Engine (OlympeEngine.exe)
# ============================================================================
if(BUILD_RUNTIME_ENGINE)
    file(GLOB_RECURSE RUNTIME_SOURCES 
        "Source/OlympeEngine.cpp"
        "Source/GameEngine.cpp"
        "Source/GameState.cpp"
        "Source/World.cpp"
        "Source/VideoGame.cpp"
        "Source/PanelManager.cpp"
        "Source/InputsInspectorPanel.cpp"
        "Source/ECS_Systems.cpp"
        "Source/ECS_Systems_AI.cpp"
        "Source/ECS_Systems_Camera.cpp"
        "Source/ECS_Systems_Rendering_Camera.cpp"
        "Source/ECS_Systems_Animation.cpp"
        "Source/Animation/AnimationManager.cpp"
        "Source/engine_utils.cpp"
        "Source/vector.cpp"
        "Source/Sprite.cpp"
        "Source/PrefabScanner.cpp"
        "Source/ComponentDefinition.cpp"
        "Source/PrefabFactory.cpp"
        "Source/Rendering/IsometricRenderer.cpp"
        "Source/Animation/*.cpp"
        "Source/Editor/AnimationEditorWindow.cpp"
    )
    
    add_executable(OlympeEngine ${RUNTIME_SOURCES})
    
    target_link_libraries(OlympeEngine PRIVATE OlympeCore)
    target_compile_definitions(OlympeEngine PRIVATE OLYMPE_RUNTIME_MODE)
    
    set_target_properties(OlympeEngine PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )
    
    # Copy SDL3 DLL to output directory (Windows only)
    if(WIN32)
        add_custom_command(TARGET OlympeEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:OlympeEngine>
        )
    endif()
endif()

# ============================================================================
# Exécutable 2: Blueprint Editor Standalone (OlympeBlueprintEditor.exe)
# ============================================================================
if(BUILD_BLUEPRINT_EDITOR_STANDALONE)
    add_executable(OlympeBlueprintEditor
        Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp
    )
    
    target_link_libraries(OlympeBlueprintEditor PRIVATE OlympeCore)
    target_compile_definitions(OlympeBlueprintEditor PRIVATE OLYMPE_BLUEPRINT_EDITOR_STANDALONE_MODE)
    
    set_target_properties(OlympeBlueprintEditor PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )
    
    # Copy SDL3 DLL to output directory (Windows only)
    if(WIN32)
        add_custom_command(TARGET OlympeBlueprintEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:OlympeBlueprintEditor>
        )
    endif()
    
    # Copy Blueprints directory to output
    add_custom_command(TARGET OlympeBlueprintEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Blueprints
        $<TARGET_FILE_DIR:OlympeBlueprintEditor>/Blueprints
    )
endif()

# ============================================================================
# Unit Tests (opt-in; no platform-specific link dependencies required)
# ============================================================================

if(BUILD_TESTS)
    enable_testing()

    add_executable(OlympeTests
        Tests/TaskSystem/TaskGraphLoaderTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME TaskGraphLoaderTest
        COMMAND OlympeTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_executable(OlympeAssetManagerTests
        Tests/TaskSystem/AssetManagerTaskGraphTest.cpp
        Tests/TestStubs.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeAssetManagerTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeAssetManagerTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME AssetManagerTaskGraphTest
        COMMAND OlympeAssetManagerTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeTaskSystemTests - Phase 1.4: TaskSystem skeleton
    # -------------------------------------------------------------------------
    add_executable(OlympeTaskSystemTests
        Tests/TaskSystem/TaskSystemTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeTaskSystemTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeTaskSystemTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME TaskSystemTest
        COMMAND OlympeTaskSystemTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeLocalBlackboardTests - Phase 2.A: LocalBlackboard
    # -------------------------------------------------------------------------
    add_executable(OlympeLocalBlackboardTests
        Tests/TaskSystem/LocalBlackboardTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeLocalBlackboardTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeLocalBlackboardTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME LocalBlackboardTest
        COMMAND OlympeLocalBlackboardTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeLocalBlackboardPersistenceTests - Phase 2.D: Serialize/Deserialize
    # -------------------------------------------------------------------------
    add_executable(OlympeLocalBlackboardPersistenceTests
        Tests/TaskSystem/LocalBlackboardPersistenceTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeLocalBlackboardPersistenceTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeLocalBlackboardPersistenceTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME LocalBlackboardPersistenceTest
        COMMAND OlympeLocalBlackboardPersistenceTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeAtomicTaskRegistryTests - Phase 2.B: AtomicTask API & Registry
    # -------------------------------------------------------------------------
    add_executable(OlympeAtomicTaskRegistryTests
        Tests/TaskSystem/AtomicTaskRegistryTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/AtomicTasks/Task_LogMessage.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeAtomicTaskRegistryTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeAtomicTaskRegistryTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME AtomicTaskRegistryTest
        COMMAND OlympeAtomicTaskRegistryTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeTaskSystemRunningTests - Phase 2.C: AtomicTask lifecycle & Abort()
    # -------------------------------------------------------------------------
    add_executable(OlympeTaskSystemRunningTests
        Tests/TaskSystem/TaskSystemRunningTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeTaskSystemRunningTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeTaskSystemRunningTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME TaskSystemRunningTest
        COMMAND OlympeTaskSystemRunningTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeMoveToLocalBBTests - Phase 3.1: Task_MoveToLocation headless tests
    # -------------------------------------------------------------------------
    add_executable(OlympeMoveToLocalBBTests
        Tests/TaskSystem/Task_MoveToLocation_LocalBBTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp
    )

    target_include_directories(OlympeMoveToLocalBBTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeMoveToLocalBBTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME MoveToLocalBBTest
        COMMAND OlympeMoveToLocalBBTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeTaskSystemE2ETests - Phase 3.1: MoveTo->Wait->SetVariable E2E test
    # -------------------------------------------------------------------------
    add_executable(OlympeTaskSystemE2ETests
        Tests/TaskSystem/TaskSystem_E2E_ExampleGraph.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp
        Source/TaskSystem/AtomicTasks/Utility/Task_Wait.cpp
        Source/TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.cpp
    )

    target_include_directories(OlympeTaskSystemE2ETests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeTaskSystemE2ETests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME TaskSystemE2ETest
        COMMAND OlympeTaskSystemE2ETests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeRequestPathfindingTests - Phase 3.2: Async PathfindingManager tests
    # -------------------------------------------------------------------------
    add_executable(OlympeRequestPathfindingTests
        Tests/TaskSystem/Task_RequestPathfindingTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/AtomicTasks/AI/Task_RequestPathfinding.cpp
        Source/TaskSystem/Pathfinding/PathfindingManager.cpp
    )

    target_include_directories(OlympeRequestPathfindingTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeRequestPathfindingTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    if(UNIX)
        target_link_libraries(OlympeRequestPathfindingTests PRIVATE pthread)
    endif()

    add_test(
        NAME RequestPathfindingTest
        COMMAND OlympeRequestPathfindingTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeMoveToWorldModeTests - Phase 4: Task_MoveToLocation ECS (World) mode
    # -------------------------------------------------------------------------
    add_executable(OlympeMoveToWorldModeTests
        Tests/TaskSystem/Task_MoveToLocation_WorldMode_Test.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp
        Source/ECS/Components/PositionComponent.cpp
        Source/ECS/Components/MovementComponent.cpp
    )

    target_include_directories(OlympeMoveToWorldModeTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeMoveToWorldModeTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME MoveToWorldModeTest
        COMMAND OlympeMoveToWorldModeTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeSetVariableTests - Phase 4: Task_SetVariable error paths + short ID
    # -------------------------------------------------------------------------
    add_executable(OlympeSetVariableTests
        Tests/TaskSystem/Task_SetVariable_Test.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
        Source/TaskSystem/AtomicTasks/Blackboard/Task_SetVariable.cpp
    )

    target_include_directories(OlympeSetVariableTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeSetVariableTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME SetVariableTest
        COMMAND OlympeSetVariableTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeEditorBridgeTests - Phase 4: TaskExecutionBridge install/uninstall
    # -------------------------------------------------------------------------
    add_executable(OlympeEditorBridgeTests
        Tests/Editor/EditorRuntimeBridgeTest.cpp
        Tests/TestStubs.cpp
        Source/TaskSystem/TaskExecutionBridge.cpp
        Source/TaskSystem/TaskSystem.cpp
        Source/TaskSystem/AtomicTaskRegistry.cpp
        Source/TaskSystem/LocalBlackboard.cpp
        Source/ECS/Components/TaskRunnerComponent.cpp
        Source/Core/AssetManager.cpp
        Source/TaskSystem/TaskGraphLoader.cpp
        Source/TaskSystem/TaskGraphTemplate.cpp
        Source/TaskSystem/TaskGraphTypes.cpp
    )

    target_include_directories(OlympeEditorBridgeTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeEditorBridgeTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME EditorRuntimeBridgeTest
        COMMAND OlympeEditorBridgeTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # -------------------------------------------------------------------------
    # OlympeClipboardTests - Phase 4: NodeGraph CRUD / paste round-trip
    # -------------------------------------------------------------------------
    add_executable(OlympeClipboardTests
        Tests/Editor/ClipboardCopyPasteTest.cpp
        Tests/TestStubs.cpp
        Source/BlueprintEditor/BTNodeGraphManager.cpp
    )

    target_include_directories(OlympeClipboardTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/BlueprintEditor
        ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
        ${CMAKE_SOURCE_DIR}/SDL/include
        ${CMAKE_SOURCE_DIR}/SDL/include/SDL3
    )

    set_target_properties(OlympeClipboardTests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME ClipboardCopyPasteTest
        COMMAND OlympeClipboardTests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    message(STATUS "  Unit Tests: ON")
else()
    message(STATUS "  Unit Tests: OFF (enable with -DBUILD_TESTS=ON)")
endif()

# ============================================================================
# Installation
# ============================================================================
if(BUILD_RUNTIME_ENGINE)
    install(TARGETS OlympeEngine RUNTIME DESTINATION bin)
endif()

if(BUILD_BLUEPRINT_EDITOR_STANDALONE)
    install(TARGETS OlympeBlueprintEditor RUNTIME DESTINATION bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Blueprints DESTINATION bin)
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "============================================")
message(STATUS "  Olympe Engine Build Configuration")
message(STATUS "============================================")
message(STATUS "  C++ Standard: C++14")
message(STATUS "  Runtime Engine: ${BUILD_RUNTIME_ENGINE}")
message(STATUS "  Blueprint Editor Standalone: ${BUILD_BLUEPRINT_EDITOR_STANDALONE}")
message(STATUS "============================================")
