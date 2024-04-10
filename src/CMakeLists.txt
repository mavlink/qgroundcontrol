qt_add_library(qgc STATIC
	CmdLineOptParser.cc
	CmdLineOptParser.h
	QGCApplication.cc
	QGCApplication.h
	QGCConfig.h
	QGCToolbox.cc
	QGCToolbox.h
)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS APP_VERSION_STR="${APP_VERSION_STR}")

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
	PUBLIC
		Qt6::QuickControls2
		Qt6::QuickWidgets
		Qt6::Widgets
		Qt6::Core5Compat

		ADSB
		AirLink
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
		QGCLocation
		Settings
		Terrain
		Utilities
		UTMSP
		Vehicle
		VehicleSetup
		VideoManager
		Viewer3D
)

target_include_directories(qgc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_sources(qgc
        PUBLIC
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cpp
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.cc
            ${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.h
    )

    target_link_libraries(qgc
        PUBLIC
            qtandroidserialport
    )

    target_include_directories(qgc PUBLIC ${CMAKE_SOURCE_DIR}/android/src)
else()
	target_sources(qgc
		PRIVATE
			RunGuard.cc
			RunGuard.h
	)
endif()
