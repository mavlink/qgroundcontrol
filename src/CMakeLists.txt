find_package(Qt6
	REQUIRED
	COMPONENTS
		Core
		Core5Compat
		QuickControls2
		QuickWidgets
		Widgets
)

qt_add_library(QGC STATIC
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
add_subdirectory(UI)
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
target_link_libraries(QGC
	PRIVATE
		Qt6::Quick
		Qt6::QuickControls2
		Qt6::QuickWidgets
		Qt6::Core5Compat
		Qt6::Bluetooth
		API
		ADSB
		AirLink
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
		GPS
		Joystick
		MAVLink
		MissionManager
		# PlanView
		PositionManager
		QmlControls
		Settings
		Terrain
		UI
		Utilities
		UTMSP
		Vehicle
		VehicleSetup
		Viewer3D
	PUBLIC
		Qt6::Core
		Qt6::CorePrivate
		Qt6::Widgets

		QGCLocation
		VideoManager
)

if(QGC_CUSTOM_BUILD)
    target_link_libraries(QGC PRIVATE custom)
endif()

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
	target_sources(QGC
		PRIVATE
			${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.cc
			${CMAKE_SOURCE_DIR}/android/src/AndroidInterface.h
		PUBLIC
			${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cpp
	)

	target_link_libraries(QGC
		PRIVATE
			Qt6::CorePrivate
		PUBLIC
			Qt6::Core
                        Comms
                        Joystick
	)

	target_include_directories(QGC PUBLIC ${CMAKE_SOURCE_DIR}/android/src)
else()
	target_sources(QGC
		PRIVATE
			RunGuard.cc
			RunGuard.h
	)
endif()

target_precompile_headers(QGC PRIVATE ${CMAKE_SOURCE_DIR}/src/pch.h)
