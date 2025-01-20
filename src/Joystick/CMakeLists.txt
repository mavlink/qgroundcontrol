find_package(Qt6 REQUIRED COMPONENTS Core Qml QmlIntegration)

qt_add_library(Joystick STATIC
    Joystick.cc
    Joystick.h
    JoystickManager.cc
    JoystickManager.h
)

target_link_libraries(Joystick
    PRIVATE
        Qt6::Qml
        FirmwarePlugin
        Gimbal
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
        QmlControls
)

target_include_directories(Joystick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_sources(Joystick
        PRIVATE
            JoystickAndroid.cc
            JoystickAndroid.h
    )

    target_link_libraries(Joystick PRIVATE Qt6::CorePrivate)
    return()
endif()

target_sources(Joystick
    PRIVATE
        JoystickSDL.cc
        JoystickSDL.h
)

target_compile_definitions(Joystick PRIVATE QGC_SDL_JOYSTICK SDL_MAIN_HANDLED)

# qt_add_qml_module(Joystick
#     URI QGroundControl.JoystickManager
#     VERSION 1.0
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )

#===========================================================================#

include(CPM)
CPMAddPackage(
    NAME sdl_gamecontrollerdb
    GITHUB_REPOSITORY mdqinc/SDL_GameControllerDB
    GIT_TAG master
)

set_source_files_properties(${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt PROPERTIES QT_RESOURCE_ALIAS gamecontrollerdb.txt)

qt_add_resources(Joystick "gamecontrollerdb.txt"
    PREFIX "/db/mapping/joystick"
    FILES ${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt
)

#===========================================================================#

# https://github.com/libsdl-org/SDL/releases/download/prerelease-3.1.10/SDL3-3.1.10-win32-x64.zip
# https://github.com/libsdl-org/SDL/releases/download/prerelease-3.1.10/SDL3-devel-3.1.10-android.zip
# https://github.com/libsdl-org/SDL/releases/download/prerelease-3.1.10/SDL3-3.1.10.dmg

if(WIN32)
    set(SDL2_EXTRA_OPTIONS
        "SDL_DIRECTX OFF"
        "SDL_XINPUT ON"
    )
endif()

CPMFindPackage(
    NAME sdl2
    VERSION 2.0.20
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-2.30.11
    OPTIONS
        "SDL2_DISABLE_INSTALL ON"
        "SDL2_DISABLE_INSTALL ON"
        "SDL2_DISABLE_UNINSTALL ON"
        "SDL2_DISABLE_SDL2MAIN ON"
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST OFF"
        "SDL_ATOMIC ON"
        "SDL_AUDIO OFF"
        "SDL_CPUINFO ON"
        "SDL_EVENTS ON"
        "SDL_FILE OFF"
        "SDL_FILESYSTEM OFF"
        "SDL_HAPTIC ON"
        "SDL_HIDAPI ON"
        "SDL_JOYSTICK ON"
        "SDL_LOADSO ON"
        "SDL_LOCALE OFF"
        "SDL_MISC OFF"
        "SDL_POWER ON"
        "SDL_RENDER OFF"
        "SDL_SENSOR OFF"
        "SDL_THREADS ON"
        "SDL_TIMERS OFF"
        "SDL_VIDEO OFF"
        "SDL_3DNOW OFF"
        "SDL_DBUS OFF"
        "SDL_IBUS OFF"
        "SDL_MMX OFF"
        "SDL_VIRTUAL_JOYSTICK ON"
        "${SDL2_EXTRA_OPTIONS}"
)

if(TARGET SDL2::SDL2-static)
    target_link_libraries(Joystick PRIVATE SDL2::SDL2-static)
endif()
