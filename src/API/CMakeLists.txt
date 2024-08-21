find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_add_library(API STATIC
    QGCCorePlugin.cc
    QGCCorePlugin.h
    QGCOptions.cc
    QGCOptions.h
    QGCSettings.cc
    QGCSettings.h
    QmlComponentInfo.cc
    QmlComponentInfo.h
)

target_link_libraries(API
    PRIVATE
        Qt6::Qml
        FactSystem
        GStreamerReceiver
        Joystick
        QtMultimediaReceiver
        Settings
        Utilities
        VideoManager
        VideoReceiver
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Comms
        QGC
        QmlControls
)

target_include_directories(API PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
