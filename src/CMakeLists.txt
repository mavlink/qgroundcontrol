find_package(Qt6
    REQUIRED
    COMPONENTS
        Core
        Core5Compat
        QuickControls2
        QuickWidgets
        Widgets
)

qt_add_library(qgc STATIC
	CmdLineOptParser.cc
	CmdLineOptParser.h
	QGCApplication.cc
	QGCApplication.h
	QGCConfig.h
	QGCToolbox.cc
	QGCToolbox.h
)

add_subdirectory(ADSB)
add_subdirectory(AirLink)
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
add_subdirectory(Terrain)
add_subdirectory(ui)
add_subdirectory(Utilities)
add_subdirectory(UTMSP)
add_subdirectory(Vehicle)
add_subdirectory(VehicleSetup)
add_subdirectory(VideoManager)
add_subdirectory(VideoReceiver)
add_subdirectory(Viewer3D)

#######################################################
#               Linking Libraries
#######################################################
target_link_libraries(qgc
	PRIVATE
		Qt6::Quick
		Qt6::QuickControls2
		Qt6::QuickWidgets
		Qt6::Core5Compat
		Qt6::Bluetooth
                api
                ADSB
                AirLink
                AnalyzeView
                Audio
                AutoPilotPlugins
                Camera
                comm
                FactSystem
                FirmwarePlugin
                FlightMap
                FollowMe
                gps
                Joystick
                MissionManager
                QmlControls
                Settings
                Terrain
                Utilities
                UTMSP
                Vehicle
                VehicleSetup
                Viewer3D
	PUBLIC
		Qt6::Core
		Qt6::CorePrivate
		Qt6::Widgets

                PositionManager
                QGCLocation
                VideoManager
)

target_include_directories(qgc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_sources(qgc
        PRIVATE
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.cc
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.h
        PUBLIC
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cpp
    )

    target_link_libraries(qgc
        PRIVATE
            Qt6::CorePrivate
        PUBLIC
            Qt6::Core
            comm
            Joystick
    )

    target_include_directories(qgc PUBLIC ${CMAKE_SOURCE_DIR}/android/src)
else()
	target_sources(qgc
		PRIVATE
			RunGuard.cc
			RunGuard.h
	)
endif()
