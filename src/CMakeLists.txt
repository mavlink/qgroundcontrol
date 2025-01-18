find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml QuickControls2 Widgets)

qt_add_library(QGC STATIC
    CmdLineOptParser.cc
    CmdLineOptParser.h
    QGCApplication.cc
    QGCApplication.h
)

add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(API)
add_subdirectory(Audio)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
# add_subdirectory(FirstRunPromptDialogs)
# add_subdirectory(FlightDisplay)
# add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(Geo)
add_subdirectory(Gimbal)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MAVLink)
add_subdirectory(MissionManager)
# add_subdirectory(PlanView)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(QtLocationPlugin)
add_subdirectory(Settings)
add_subdirectory(Terrain)
# add_subdirectory(UI)
add_subdirectory(Utilities)
add_subdirectory(UTMSP)
add_subdirectory(Vehicle)
add_subdirectory(VehicleSetup)
add_subdirectory(VideoManager)
add_subdirectory(Viewer3D)

#######################################################
#               Linking Libraries
#######################################################
target_link_libraries(QGC
    PRIVATE
        Qt6::CorePrivate
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        API
        AnalyzeView
        Audio
        AutoPilotPlugins
        Camera
        Comms
        CommonAutoPilotPlugin
        FactSystem
        FirmwarePlugin
        # FirstRunPromptDialogs
        # FlightMap
        # FlightDisplay
        FollowMe
        Gimbal
        GPS
        GStreamerReceiver
        Joystick
        MAVLink
        MissionManager
        # PlanView
        PositionManager
        QGCLocation
        QmlControls
        Settings
        # UI
        Utilities
        Vehicle
        VehicleSetup
        Viewer3D
        VideoManager
    PUBLIC
        Qt6::Core
        Qt6::Widgets
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(QGC
    PRIVATE
        <QtCore/QObject>
        <QtCore/QString>
)

if(ANDROID)
    target_sources(QGC
        PRIVATE
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.cc
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.h
    )

    target_link_libraries(QGC
        PRIVATE
            Qt6::CorePrivate
            Utilities
        PUBLIC
            Qt6::Core
    )

    target_include_directories(QGC PUBLIC ${CMAKE_SOURCE_DIR}/android/src)
endif()
