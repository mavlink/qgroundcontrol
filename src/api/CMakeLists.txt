find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_add_library(api STATIC
	QGCCorePlugin.cc
	QGCCorePlugin.h
	QGCOptions.cc
	QGCOptions.h
	QGCSettings.cc
	QGCSettings.h
	QmlComponentInfo.cc
	QmlComponentInfo.h
)

target_link_libraries(api
	PRIVATE
		Qt6::Qml
		FactSystem
		Joystick
		Settings
		Utilities
		VideoReceiver
		VideoManager
	PUBLIC
		Qt6::Core
		Qt6::Gui
		comm
		qgc
		QmlControls
)

target_include_directories(api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
