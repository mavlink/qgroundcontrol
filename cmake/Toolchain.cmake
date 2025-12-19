# ----------------------------------------------------------------------------
# QGroundControl Toolchain Configuration
# Sets compiler, linker, and build tool settings
# ----------------------------------------------------------------------------

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

# ----------------------------------------------------------------------------
# Build Configuration
# ----------------------------------------------------------------------------
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Unity builds can improve compile times but may cause issues
# Enable with caution - conflicts with CMAKE_EXPORT_COMPILE_COMMANDS
# set(CMAKE_EXPORT_BUILD_DATABASE ON)
# set(CMAKE_UNITY_BUILD ON)
# set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)

# ----------------------------------------------------------------------------
# Security & Optimization Settings
# ----------------------------------------------------------------------------

qgc_enable_pie()

if(NOT LINUX)
    qgc_enable_ipo()
endif()

# ----------------------------------------------------------------------------
# Compiler & Linker Optimizations
# ----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Use faster alternative linkers on non-Apple platforms
    if(NOT APPLE)
        qgc_set_linker()
    endif()

    # Link-Time Optimization (LTO) for Release builds
    # Thin LTO provides faster incremental builds (Clang-specific)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_link_options("$<$<CONFIG:Release>:-flto=thin>")
    else()
        # GCC uses standard LTO (handled by qgc_enable_ipo above)
    endif()
elseif(MSVC)
    # MSVC-specific optimizations
    add_link_options("$<$<CONFIG:Release>:/LTCG:INCREMENTAL>")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
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

# ----------------------------------------------------------------------------
# Platform Detection Helpers
# ----------------------------------------------------------------------------
if(APPLE AND NOT IOS)
    set(MACOS TRUE)
endif()
