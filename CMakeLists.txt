cmake_minimum_required(VERSION 3.22)

project(meta-root)

# -----------------------------------------------------------------------------
# Build type (Debug|Release|RelWithDebInfo)
# -----------------------------------------------------------------------------

# default for single-config generators
set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "${DEFAULT_BUILD_TYPE}"
      CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
# Core options
# -----------------------------------------------------------------------------

option(META_ENABLE_TESTS "" ON)
option(META_ENABLE_UNITTESTS "Enable GoogleTest unit tests" OFF)
option(META_ENABLE_GLM_TYPES "Enable built-in GLM type registrations" ON)
option(META_ENABLE_COLOR_GRADIENT_TYPES
       "Enable built-in color gradient type registrations" ON)
option(META_ENABLE_ARRAY_TYPES "Enable built-in array type registrations" ON)

# -----------------------------------------------------------------------------
# UI backends
# -----------------------------------------------------------------------------

option(META_ENABLE_FTXUI_UI "Enable FTXUI UI backend" OFF)
option(META_ENABLE_QT_UI "Enable QT UI backend" OFF)

# -----------------------------------------------------------------------------
# Status output
# -----------------------------------------------------------------------------

message(STATUS "┌─ Meta configuration ────────────────")
message(STATUS "│ META_ENABLE_GLM_TYPES:            ${META_ENABLE_GLM_TYPES}")
message(
  STATUS
    "│ META_ENABLE_COLOR_GRADIENT_TYPES: ${META_ENABLE_COLOR_GRADIENT_TYPES}")
message(STATUS "│ META_ENABLE_ARRAY_TYPES:          ${META_ENABLE_ARRAY_TYPES}")
message(STATUS "│ META_ENABLE_FTXUI_UI:             ${META_ENABLE_FTXUI_UI}")
message(STATUS "│ META_ENABLE_QT_UI:                ${META_ENABLE_QT_UI}")
message(STATUS "│ META_ENABLE_TESTS:                ${META_ENABLE_TESTS}")
message(STATUS "│ META_ENABLE_UNITTESTS:            ${META_ENABLE_UNITTESTS}")
message(STATUS "│ CMAKE_BUILD_TYPE:                 ${CMAKE_BUILD_TYPE}")
message(STATUS "└─────────────────────────────────────")

# -----------------------------------------------------------------------------
# C++ standard
# -----------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")
endif()

# -----------------------------------------------------------------------------
# Output directory
# -----------------------------------------------------------------------------

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

if(META_ENABLE_GLM_TYPES)
  find_package(glm REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Core library
# -----------------------------------------------------------------------------

add_subdirectory(Meta)

# -----------------------------------------------------------------------------
# Optional UI backends
# -----------------------------------------------------------------------------

if(META_ENABLE_FTXUI_UI OR META_ENABLE_QT_UI)
  add_subdirectory(MetaUI/common)
endif()

if(META_ENABLE_FTXUI_UI)
  add_subdirectory(MetaUI/ftxui)
endif()

if(META_ENABLE_QT_UI)
  add_subdirectory(MetaUI/qt)
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

if(META_ENABLE_TESTS)
  add_subdirectory(tests)
endif()
