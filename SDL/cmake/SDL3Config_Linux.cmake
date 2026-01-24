# SDL3 CMake configuration file for Linux (header-only for compilation)

cmake_minimum_required(VERSION 3.0)

set(SDL3_FOUND TRUE)

get_filename_component(_sdl3_prefix "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_sdl3_include_dirs "${_sdl3_prefix}/include")

# Create header-only target for compilation testing
if(NOT TARGET SDL3::Headers)
    add_library(SDL3::Headers INTERFACE IMPORTED)
    set_target_properties(SDL3::Headers
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl3_include_dirs}"
    )
endif()

if(NOT TARGET SDL3::SDL3)
    add_library(SDL3::SDL3 INTERFACE IMPORTED)
    set_target_properties(SDL3::SDL3
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl3_include_dirs}"
    )
endif()

set(SDL3_LIBRARIES SDL3::SDL3)
set(SDL3_FOUND TRUE)
