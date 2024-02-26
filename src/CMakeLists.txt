#######################################################
#                   Main QGC library
#######################################################
project(qgc)

#######################################################
#               Directory Inclusion
#######################################################
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Qt6Location_PRIVATE_INCLUDE_DIRS}
)

#######################################################
#             Source Files
#######################################################

qt_add_library(${PROJECT_NAME} STATIC
        CmdLineOptParser.cc
        CmdLineOptParser.h
        JsonHelper.cc
        JsonHelper.h
        main.cc
        QGC.cc
        QGC.h
        QGCApplication.cc
        QGCApplication.h
        QGCConfig.h
        QGCToolbox.cc
        QGCToolbox.h
        RunGuard.cc
        RunGuard.h
)

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS APP_VERSION_STR="${APP_VERSION_STR}")

# Subdirectories
add_subdirectory(ui)

add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(api)
add_subdirectory(Audio)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(comm)
add_subdirectory(Compression)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
add_subdirectory(FlightDisplay)
add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(Geo)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MissionManager)
add_subdirectory(PlanView)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(QtLocationPlugin)
add_subdirectory(Settings)
if (NOT ${QGC_AIRLINK_DISABLED})
    add_subdirectory(AirLink)
endif()
add_subdirectory(Terrain)
add_subdirectory(Tools)
add_subdirectory(UTMSP)
add_subdirectory(Vehicle)
add_subdirectory(VehicleSetup)
add_subdirectory(VideoManager)
add_subdirectory(VideoReceiver)
add_subdirectory(Viewer3D)

#######################################################
#               Linking Libraries
#######################################################
target_link_libraries(${PROJECT_NAME}
        PRIVATE
            Qt6::QuickControls2
        PUBLIC
            Qt6::QuickWidgets
            Qt6::Widgets
            Qt6::Core5Compat

            ADSB
            AnalyzeView
            api
            Audio
            AutoPilotPlugins
            Camera
            comm
            compression
            FactSystem
            FirmwarePlugin
            FlightMap
            FollowMe
            Geo
            gps
            Joystick
            MissionManager
            PositionManager
            QmlControls
            QtLocationPlugin
            Settings
            Terrain
            Tools
            UTMSP
            Vehicle
            VehicleSetup
            VideoManager
            Viewer3D
)

if (NOT ${QGC_AIRLINK_DISABLED})
    target_link_libraries(${PROJECT_NAME} PUBLIC AirLink)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC QGC_AIRLINK_DISABLED)
endif()

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/ui/ui_autogen/include # HACK: AUTOUIC paths not inheriting?
        ${CMAKE_CURRENT_BINARY_DIR}/ui/ui_autogen/include_Debug
        ${CMAKE_CURRENT_BINARY_DIR}/qgc_autogen/include # HACK: AUTOUIC paths not inheriting?
        ${CMAKE_CURRENT_BINARY_DIR}/qgc_autogen/include_Debug
)
