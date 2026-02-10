# ----------------------------------------------------------------------------
# QGroundControl Toolchain Configuration
# Sets compiler, linker, and build tool settings
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# Platform Detection Helpers
# ----------------------------------------------------------------------------
if(APPLE AND NOT IOS)
    set(MACOS TRUE)
endif()

# ----------------------------------------------------------------------------
# C++ Standard Requirements
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------
# Qt-specific Automation
# ----------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT DEFINED CMAKE_AUTOGEN_PARALLEL)
    cmake_host_system_information(RESULT _nproc QUERY NUMBER_OF_LOGICAL_CORES)
    set(CMAKE_AUTOGEN_PARALLEL ${_nproc})
endif()

# ----------------------------------------------------------------------------
# Build Configuration
# ----------------------------------------------------------------------------
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include compiler warnings configuration
include(CompilerWarnings)

# set(CMAKE_EXPORT_BUILD_DATABASE ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT WIN32)
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
        SYMBOLIC
    )
endif()

if(QGC_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# ----------------------------------------------------------------------------
# Static Analysis (clang-tidy)
# ----------------------------------------------------------------------------
if(QGC_ENABLE_CLANG_TIDY)
    find_program(QGC_CLANG_TIDY_PROGRAM NAMES clang-tidy)
    if(QGC_CLANG_TIDY_PROGRAM)
        set(CMAKE_CXX_CLANG_TIDY "${QGC_CLANG_TIDY_PROGRAM}" CACHE STRING "clang-tidy executable" FORCE)
        message(STATUS "QGC: clang-tidy enabled (${QGC_CLANG_TIDY_PROGRAM})")
    else()
        message(WARNING "QGC: QGC_ENABLE_CLANG_TIDY is ON but clang-tidy not found")
    endif()
endif()

# ----------------------------------------------------------------------------
# Security & Optimization Settings
# ----------------------------------------------------------------------------

qgc_enable_pie()
qgc_enable_ipo()

# ----------------------------------------------------------------------------
# Compiler & Linker Optimizations
# ----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Use faster alternative linkers on non-Apple platforms
    if(NOT APPLE)
        qgc_set_linker()
    endif()

    # LTO is handled by qgc_enable_ipo() above
elseif(MSVC)
    # MSVC-specific optimizations
    add_link_options("$<$<CONFIG:Release>:/LTCG:INCREMENTAL>")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ----------------------------------------------------------------------------
# Link Job Pool (Ninja only)
# ----------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Ninja")
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_pool=${QGC_LINK_PARALLEL_LEVEL})
    set(CMAKE_JOB_POOL_LINK link_pool)
endif()

# ----------------------------------------------------------------------------
# Install Configuration
# ----------------------------------------------------------------------------

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(LINUX)
        # Linux uses AppDir structure for AppImage packaging
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/AppDir/usr" CACHE PATH "Install path prefix for AppImage" FORCE)
    else()
        # Other platforms use staging directory
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/staging" CACHE PATH "Install path prefix" FORCE)
    endif()
endif()

# ----------------------------------------------------------------------------
# Cross-Compilation Configuration
# ----------------------------------------------------------------------------
if(CMAKE_CROSSCOMPILING)
    if(NOT DEFINED QT_HOST_PATH OR QT_HOST_PATH STREQUAL "")
        message(FATAL_ERROR "Cross-compilation requires QT_HOST_PATH to be defined and set to a valid Qt host installation path")
    endif()

    if(NOT IS_DIRECTORY "${QT_HOST_PATH}")
        message(FATAL_ERROR "Cross-compilation QT_HOST_PATH is not a valid directory: ${QT_HOST_PATH}")
    endif()

    if(ANDROID)
        # Android cross-compilation: search both target and host paths
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    endif()
endif()
