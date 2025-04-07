# qt_add_qml_module(QGC
#     URI QGroundControl
#     VERSION 1.0
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        main.cc
        CmdLineOptParser.cc
        CmdLineOptParser.h
        QGCApplication.cc
        QGCApplication.h
)

if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            RunGuard.cc
            RunGuard.h
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::Charts
        Qt6::Concurrent
        Qt6::Core
        Qt6::Core5Compat
        Qt6::CorePrivate
        Qt6::Gui
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Multimedia
        Qt6::MultimediaQuickPrivate
        Qt6::Network
        Qt6::Positioning
        Qt6::PositioningPrivate
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sensors
        Qt6::Svg
        Qt6::TextToSpeech
        Qt6::Widgets
        Qt6::Xml
        QGCLocation
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cc
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.cc
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.h
    )

    target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
            Qt6::Core
            Qt6::CorePrivate
    )

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src)
endif()

add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(API)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
# add_subdirectory(FirstRunPromptDialogs)
# add_subdirectory(FlightDisplay)
# add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
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
add_subdirectory(VideoManager)
add_subdirectory(Viewer3D)

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
        <QtCore/QObject>
        <QtNetwork/QNetworkReply>
        <QtPositioning/QGeoCoordinate>
)
