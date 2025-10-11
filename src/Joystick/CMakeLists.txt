# ============================================================================
# Joystick Module
# Game controller and joystick input support
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Joystick.cc
        Joystick.h
        JoystickManager.cc
        JoystickManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Android Joystick Implementation
# ----------------------------------------------------------------------------
if(ANDROID)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            JoystickAndroid.cc
            JoystickAndroid.h
    )
    return()
endif()

# ----------------------------------------------------------------------------
# SDL Joystick Implementation (Desktop)
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JoystickSDL.cc
        JoystickSDL.h
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_SDL_JOYSTICK)

if(SDL_GAMECONTROLLERCONFIG)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SDL_GAMECONTROLLERCONFIG=\"${SDL_GAMECONTROLLERCONFIG}\")
endif()

# ============================================================================
# SDL Game Controller Database
# ============================================================================

CPMAddPackage(
    NAME sdl_gamecontrollerdb
    GITHUB_REPOSITORY mdqinc/SDL_GameControllerDB
    GIT_TAG master
    DOWNLOAD_ONLY
)

set(SDL_GAMECONTROLLERDB_PATH "${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt")

qgc_set_qt_resource_alias("${SDL_GAMECONTROLLERDB_PATH}")

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_sdl"
    PREFIX "/"
    FILES
        "${SDL_GAMECONTROLLERDB_PATH}"
)

# ============================================================================
# SDL3 Library Integration
# ============================================================================

# Platform-specific SDL options
set(SDL_EXTRA_OPTIONS "")
if(WIN32)
    set(SDL_EXTRA_OPTIONS
        ${SDL_EXTRA_OPTIONS}
        "SDL_DIRECTX OFF"
        "SDL_XINPUT ON"
    )
endif()
if(UNIX)
    set(SDL_EXTRA_OPTIONS
        ${SDL_EXTRA_OPTIONS}
        "SDL_UNIX_CONSOLE_BUILD ON"
    )
endif()

# Video only needed on macOS
if(MACOS)
    set(SDL_VIDEO_FLAG "ON")
else()
    set(SDL_VIDEO_FLAG "OFF")
endif()

CPMAddPackage(
    NAME SDL3
    VERSION 3.2.20
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.2.20
    OPTIONS
        "SDL_INSTALL OFF"
        "SDL_UNINSTALL OFF"

        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST_LIBRARY OFF"
        "SDL_EXAMPLES OFF"

        "SDL_AUDIO OFF"
        "SDL_VIDEO ${SDL_VIDEO_FLAG}"
        "SDL_GPU OFF"
        "SDL_RENDER OFF"
        "SDL_CAMERA OFF"
        "SDL_JOYSTICK ON"
        "SDL_HAPTIC ON"
        "SDL_HIDAPI ON"
        "SDL_POWER ON"
        "SDL_SENSOR OFF"
        "SDL_DIALOG OFF"

        "SDL_CCACHE ON"
        "SDL_DBUS OFF"
        "SDL_IBUS OFF"
        "SDL_MMX OFF"
        "SDL_VIRTUAL_JOYSTICK ON"

        "${SDL_EXTRA_OPTIONS}"
)

target_compile_definitions(SDL3-static PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL3::SDL3-static)
