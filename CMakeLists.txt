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
