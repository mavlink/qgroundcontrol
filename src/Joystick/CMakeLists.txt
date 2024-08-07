find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(Joystick STATIC
    Joystick.cc
    Joystick.h
    JoystickManager.cc
    JoystickManager.h
)

if(ANDROID)
    target_sources(Joystick
        PRIVATE
            JoystickAndroid.cc
            JoystickAndroid.h
    )

    target_link_libraries(Joystick PRIVATE Qt6::CorePrivate)
else()
    include(BuildSdl2)
    build_sdl2()

    if(SDL2_FOUND)
        message(STATUS "Building JoystickSDL")

        target_sources(Joystick
            PRIVATE
                JoystickSDL.cc
                JoystickSDL.h
        )

        target_link_libraries(Joystick PRIVATE SDL2::SDL2-static)

        target_compile_definitions(Joystick PRIVATE QGC_SDL_JOYSTICK)

        include(FetchContent)
        FetchContent_Declare(sdl_db
            GIT_REPOSITORY https://github.com/mdqinc/SDL_GameControllerDB.git
            GIT_TAG master
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(sdl_db)

        set_source_files_properties(${sdl_db_SOURCE_DIR}/gamecontrollerdb.txt
            PROPERTIES
                QT_RESOURCE_ALIAS gamecontrollerdb.txt
        )
        qt_add_resources(Joystick "gamecontrollerdb.txt"
            PREFIX "/db/mapping/joystick"
            FILES ${sdl_db_SOURCE_DIR}/gamecontrollerdb.txt
        )

    endif()
endif()

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
