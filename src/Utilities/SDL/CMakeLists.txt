# ============================================================================
# SDL Utilities
# SDL platform and joystick utilities
# ============================================================================

# ============================================================================
# SDL3 Library Integration
# ============================================================================

set(SDL_EXTRA_OPTIONS "")
if(WIN32)
    set(SDL_EXTRA_OPTIONS
        ${SDL_EXTRA_OPTIONS}
        "SDL_DIRECTX OFF"
        "SDL_XINPUT ON"
    )
elseif(UNIX AND NOT ANDROID)
    set(SDL_EXTRA_OPTIONS
        ${SDL_EXTRA_OPTIONS}
        "SDL_UNIX_CONSOLE_BUILD ON"
    )
endif()

if(MACOS OR IOS OR ANDROID)
    set(SDL_VIDEO_FLAG "ON")
else()
    set(SDL_VIDEO_FLAG "OFF")
endif()

if(ANDROID)
    set(SDL_SHARED_FLAG "ON")
    set(SDL_STATIC_FLAG "OFF")
    set(SDL_EXTRA_OPTIONS
        ${SDL_EXTRA_OPTIONS}
        "SDL_ANDROID_JAR OFF"
        "SDL_HIDAPI_LIBUSB OFF"
    )
else()
    set(SDL_SHARED_FLAG "OFF")
    set(SDL_STATIC_FLAG "ON")
endif()

CPMAddPackage(
    NAME SDL3
    VERSION 3.4.0
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.4.0
    OPTIONS
        "SDL_INSTALL OFF"
        "SDL_UNINSTALL OFF"

        "SDL_SHARED ${SDL_SHARED_FLAG}"
        "SDL_STATIC ${SDL_STATIC_FLAG}"
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
        "SDL_SENSOR ON"
        "SDL_DIALOG OFF"

        "SDL_CCACHE ON"
        "SDL_DBUS OFF"
        "SDL_IBUS OFF"
        "SDL_MMX OFF"
        "SDL_VIRTUAL_JOYSTICK ON"

        "${SDL_EXTRA_OPTIONS}"
)

# Suppress compiler warnings for SDL3 dependency
if(SDL3_ADDED)
    if(ANDROID)
        qgc_disable_dependency_warnings(SDL3-shared)
    else()
        qgc_disable_dependency_warnings(SDL3-static)
    endif()
endif()

if(ANDROID)
    target_compile_definitions(SDL3-shared PRIVATE SDL_MAIN_NOIMPL SDL_MAIN_HANDLED)
    target_link_libraries(SDL3-shared PRIVATE log android)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL3::SDL3-shared)
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY
        QT_ANDROID_EXTRA_LIBS "$<TARGET_FILE:SDL3-shared>"
    )
else()
    target_compile_definitions(SDL3-static PRIVATE SDL_MAIN_NOIMPL SDL_MAIN_HANDLED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL3::SDL3-static)
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
# SDL Utility Sources
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        SDLPlatform.cc
        SDLPlatform.h
        SDLJoystick.cc
        SDLJoystick.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
