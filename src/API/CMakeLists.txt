find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml QmlIntegration)

qt_add_library(API STATIC
    QGCCorePlugin.cc
    QGCCorePlugin.h
    QGCOptions.cc
    QGCOptions.h
    QmlComponentInfo.cc
    QmlComponentInfo.h
)

target_link_libraries(API
    PRIVATE
        Qt6::Qml
        FactSystem
        GStreamerReceiver
        Joystick
        MAVLink
        QtMultimediaReceiver
        Settings
        Utilities
        VideoManager
        VideoReceiver
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::QmlIntegration
        QmlControls
)

target_include_directories(API PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(QGC_CUSTOM_BUILD)
    target_link_libraries(API PRIVATE CustomPlugin)
endif()
