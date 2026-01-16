cmake_minimum_required(VERSION 3.14)
project(TiledLevelLoader)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(TILED_LOADER_SOURCES
    src/TiledDecoder.cpp
    src/TiledLevelLoader.cpp
    src/TiledToOlympe.cpp
    src/TilesetCache.cpp
    src/TilesetParser.cpp
    src/IsometricProjection.cpp
    src/ParallaxLayerManager.cpp
    third_party/tinyxml2/tinyxml2.cpp
)

set(TILED_LOADER_HEADERS
    include/TiledStructures.h
    include/TiledDecoder.h
    include/TiledLevelLoader.h
    include/TiledToOlympe.h
    include/TilesetCache.h
    include/TilesetParser.h
    include/IsometricProjection.h
    include/ParallaxLayerManager.h
    include/TiledJsonHelper.h
    third_party/miniz/miniz.h
    third_party/tinyxml2/tinyxml2.h
)

# Create static library
add_library(TiledLevelLoader STATIC
    ${TILED_LOADER_SOURCES}
    ${TILED_LOADER_HEADERS}
)

target_include_directories(TiledLevelLoader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/third_party
    ${CMAKE_SOURCE_DIR}/Source/third_party/nlohmann
)

# Link SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(TiledLevelLoader PUBLIC SDL3::SDL3)

set_target_properties(TiledLevelLoader PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Disable warnings for third-party code
if(MSVC)
    set_source_files_properties(
        third_party/tinyxml2/tinyxml2.cpp
        PROPERTIES COMPILE_FLAGS "/W0"
    )
else()
    set_source_files_properties(
        third_party/tinyxml2/tinyxml2.cpp
        PROPERTIES COMPILE_FLAGS "-w"
    )
endif()
