# qt_add_qml_module(QGC
#     URI QGroundControl
#     VERSION 1.0
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CmdLineOptParser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CmdLineOptParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCApplication.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCApplication.h
    CACHE INTERNAL "" FORCE
)

if(NOT ANDROID AND NOT IOS)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/RunGuard.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/RunGuard.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Qt6::Concurrent
    Qt6::Core
    Qt6::Core5Compat
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::Network
    Qt6::Positioning
    Qt6::Qml
    Qt6::QmlIntegration
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Widgets
    Qt6::Xml
    CACHE INTERNAL "" FORCE
)

add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(Android)
add_subdirectory(API)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
add_subdirectory(FirstRunPromptDialogs)
add_subdirectory(FlightDisplay)
add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(Gimbal)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MAVLink)
add_subdirectory(MissionManager)
add_subdirectory(PlanView)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(Settings)
add_subdirectory(Terrain)
add_subdirectory(UI)
add_subdirectory(Utilities)
add_subdirectory(UTMSP)
add_subdirectory(Vehicle)
add_subdirectory(VideoManager)
add_subdirectory(Viewer3D)

add_subdirectory(QtLocationPlugin)
