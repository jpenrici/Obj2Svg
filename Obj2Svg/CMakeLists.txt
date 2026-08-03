cmake_minimum_required(VERSION 3.28)

project(
  Obj2Svg
  VERSION 0.1.0
  DESCRIPTION
    "Modular 3D editor core (C++26) with C-API boundary and Raylib client"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Debug"
      CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(OBJ2SVG_BUILD_TESTS "Build unit tests for editor_core" ON)
option(OBJ2SVG_BUILD_CLIENT "Build the Raylib client (EditorClient)" ON)
option(OBJ2SVG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# ---------------------------------------------------------------------------
# Common compiler warnings (applied via target in each subdirectory)
# ---------------------------------------------------------------------------
add_library(obj2svg_warnings INTERFACE)
target_compile_options(
  obj2svg_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion
                             $<$<BOOL:${OBJ2SVG_WARNINGS_AS_ERRORS}>:-Werror>)

# ---------------------------------------------------------------------------
# Subprojects
# ---------------------------------------------------------------------------
add_subdirectory(core)
add_subdirectory(api)

if(OBJ2SVG_BUILD_CLIENT)
  add_subdirectory(client)
endif()

if(OBJ2SVG_BUILD_TESTS)
  enable_testing()
endif()
