find_package(Qt6 REQUIRED COMPONENTS Core Qml)

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
        Comms
        QGC
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

target_compile_definitions(Joystick PRIVATE QGC_SDL_JOYSTICK)

FetchContent_Declare(SDL_GameControllerDB
    GIT_REPOSITORY https://github.com/mdqinc/SDL_GameControllerDB.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL_GameControllerDB)

set_source_files_properties(${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt
    PROPERTIES
        QT_RESOURCE_ALIAS gamecontrollerdb.txt
)

qt_add_resources(Joystick "gamecontrollerdb.txt"
    PREFIX "/db/mapping/joystick"
    FILES ${sdl_gamecontrollerdb_SOURCE_DIR}/gamecontrollerdb.txt
)

set(MINIMUM_SDL2_VERSION 2.30.0)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(SDL2 ${MINIMUM_SDL2_VERSION})
    if(SDL2_FOUND)
        message(STATUS "Found JoystickSDL ${SDL2_VERSION_STRING}")
        target_link_libraries(Joystick PRIVATE SDL2::SDL2)
        return()
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 IMPORTED_TARGET sdl2>=${MINIMUM_SDL2_VERSION})
            if(SDL2_FOUND)
                message(STATUS "Found JoystickSDL ${SDL2_VERSION}")
                target_link_libraries(Joystick PRIVATE PkgConfig::SDL2)
                return()
            endif()
        endif()
    endif()
endif()

message(STATUS "Building JoystickSDL")
include(FetchContent)
FetchContent_Declare(SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.7
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(SDL2_DISABLE_INSTALL ON CACHE INTERNAL "" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE INTERNAL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE INTERNAL "" FORCE)
set(SDL_SHARED OFF CACHE INTERNAL "" FORCE)
set(SDL_STATIC ON CACHE INTERNAL "" FORCE)
set(SDL_TEST OFF CACHE INTERNAL "" FORCE)

set(SDL_ATOMIC ON CACHE INTERNAL "" FORCE)
set(SDL_AUDIO OFF CACHE INTERNAL "" FORCE)
set(SDL_CPUINFO ON CACHE INTERNAL "" FORCE)
set(SDL_EVENTS ON CACHE INTERNAL "" FORCE)
set(SDL_FILE OFF CACHE INTERNAL "" FORCE)
set(SDL_FILESYSTEM OFF CACHE INTERNAL "" FORCE)
set(SDL_HAPTIC ON CACHE INTERNAL "" FORCE)
set(SDL_HIDAPI ON CACHE INTERNAL "" FORCE)
set(SDL_JOYSTICK ON CACHE INTERNAL "" FORCE)
set(SDL_LOADSO ON CACHE INTERNAL "" FORCE)
set(SDL_LOCALE OFF CACHE INTERNAL "" FORCE)
set(SDL_MISC OFF CACHE INTERNAL "" FORCE)
set(SDL_POWER ON CACHE INTERNAL "" FORCE)
set(SDL_RENDER OFF CACHE INTERNAL "" FORCE)
set(SDL_SENSOR OFF CACHE INTERNAL "" FORCE)
set(SDL_THREADS ON CACHE INTERNAL "" FORCE)
set(SDL_TIMERS OFF CACHE INTERNAL "" FORCE)
set(SDL_VIDEO OFF CACHE INTERNAL "" FORCE)

set(SDL_3DNOW OFF CACHE INTERNAL "" FORCE)
set(SDL_DBUS OFF CACHE INTERNAL "" FORCE)
set(SDL_IBUS OFF CACHE INTERNAL "" FORCE)
set(SDL_MMX OFF CACHE INTERNAL "" FORCE)
set(SDL_VIRTUAL_JOYSTICK ON CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(SDL)

target_link_libraries(Joystick PRIVATE SDL2::SDL2-static)
