cmake_minimum_required(VERSION 3.31)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/cmake/lockstep.cmake)

project(PondDev VERSION 0.1.0 LANGUAGES C CXX)

# The libraries the pond dev apps (Rudder, Grazer, CowTerm, iac, DiffView)
# share. An app adds PondDev with lockstep_add_package and gets the libraries;
# only a top-level build adds the tests.

# pond_add_version_library: each app's "<semver>+<commit>" version string.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/PondVersion.cmake)

# pond_add_line_coverage_counters and pond_write_line_coverage.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/PondLineCoverage.cmake)

if (APPLE)
    enable_language(OBJCXX)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # No source this project compiles uses C++20 modules, so scanning each one for them is wasted work.
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif ()

# PondApp and PermissionsPanel are built on eacp. An app that already added
# eacp keeps its own; otherwise PondDev brings it. Work against a local eacp
# checkout with:
#   cmake -B build -DLOCKSTEP_eacp_SOURCE=$HOME/projects/eacp
#
# eacp jp/develop: develop plus Jamie's unmerged fixes, one commit each, as Rudder pins it.
if (NOT TARGET eacp-core)
    lockstep_add_package(
            NAME eacp
            GITHUB_REPOSITORY eyalamirmusic/eacp
            GIT_TAG jp/develop
            SYSTEM YES)
endif ()

# The vim editor (TextBuffer, VimEditor) the search box is built on.
# Work against a local checkout with:
#   cmake -B build -DLOCKSTEP_text-buffer_SOURCE=$HOME/projects/text-buffer
if (NOT TARGET VimEditor)
    lockstep_add_package(
            NAME text-buffer
            GITHUB_REPOSITORY jamierpond/text-buffer
            GIT_TAG main
            SYSTEM YES)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    if (NOT TARGET NanoTest)
        lockstep_add_package(
                NAME NanoTest
                GITHUB_REPOSITORY eyalamirmusic/NanoTest
                GIT_TAG main
                SYSTEM YES)
    endif ()

    # Set after the packages, so only this project's own targets build with -Werror.
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif ()

# Matching and usage ordering for search lists: plain functions on strings
# and times, no storage, no dependencies.
add_library(FrecencySearch STATIC
        Lib/FrecencySearch/LearnedPicks.cpp
        Lib/FrecencySearch/MatchLevel.cpp
        Lib/FrecencySearch/RowOrder.cpp)
target_include_directories(FrecencySearch PUBLIC Lib)
target_compile_features(FrecencySearch PUBLIC cxx_std_20)

# A one-line vim search box over a list of rows, so Rudder, Grazer and
# CowTerm share one set of keys.
add_library(SearchKeys STATIC Lib/SearchKeys/SearchKeys.cpp)
target_include_directories(SearchKeys PUBLIC Lib)
target_compile_features(SearchKeys PUBLIC cxx_std_20)
target_link_libraries(SearchKeys PUBLIC VimEditor)

# Per-launch log files (<logs>/<startEpochMs>.log, oldest pruned) and one
# `<event> key=value ...` line format, so every app's usage reads the same way.
add_library(SessionLog STATIC
        Lib/SessionLog/EventLine.cpp
        Lib/SessionLog/SessionLog.cpp)
target_include_directories(SessionLog PUBLIC Lib)
target_compile_features(SessionLog PUBLIC cxx_std_20)

add_subdirectory(Lib/PondApp)
add_subdirectory(Lib/PermissionsPanel)

if (PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(Tests)
endif ()
