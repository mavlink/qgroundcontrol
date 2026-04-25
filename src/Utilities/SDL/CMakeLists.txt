# ============================================================================
# SDL Utilities
# SDL3 platform/joystick wrappers built as a standalone STATIC library so the
# SDL3 CPM package and gamecontrollerdb resource don't sit on the main target.
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
    VERSION 3.4.2
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.4.2
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
    set(QGC_SDL3_TARGET SDL3::SDL3-shared)
else()
    target_compile_definitions(SDL3-static PRIVATE SDL_MAIN_NOIMPL SDL_MAIN_HANDLED)
    set(QGC_SDL3_TARGET SDL3::SDL3-static)
endif()

# ============================================================================
# QGCSDL static library
# ============================================================================

qt_add_library(QGCSDL STATIC
    SDLPlatform.cc
    SDLPlatform.h
    SDLJoystick.cc
    SDLJoystick.h
)

target_link_libraries(QGCSDL
    PUBLIC
        Qt6::Core
        ${QGC_SDL3_TARGET}
    PRIVATE
        QGCLogging
)

target_include_directories(QGCSDL
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# SDL Game Controller Database
# Loaded at runtime from the Qt resource system.
# ============================================================================

CPMAddPackage(
    NAME sdl_gamecontrollerdb
    GITHUB_REPOSITORY mdqinc/SDL_GameControllerDB
    GIT_TAG 7c3baed7e78f4b85c12df22089b2b97410edec15
    DOWNLOAD_ONLY
)

set(SDL_GAMECONTROLLERDB_PATH "${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt")
qgc_set_qt_resource_alias("${SDL_GAMECONTROLLERDB_PATH}")

qt_add_resources(QGCSDL "qgcresources_sdl"
    PREFIX "/"
    FILES
        "${SDL_GAMECONTROLLERDB_PATH}"
)

# ============================================================================
# Wire into the main target
# ============================================================================

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCSDL)

if(ANDROID)
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY
        QT_ANDROID_EXTRA_LIBS "$<TARGET_FILE:SDL3-shared>"
    )
endif()
