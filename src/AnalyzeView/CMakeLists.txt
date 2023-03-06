
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		LogDownloadTest.cc
		LogDownloadTest.h
	)
endif()

if (NOT QGC_DISABLE_MAVLINK_INSPECTOR)
	set(MAVLINK_INSPECTOR_SRC
		MAVLinkInspectorController.cc
		MAVLinkInspectorController.h
	)
	set(MAVLINK_INSPECTOR_QML
		MAVLinkInspectorPage.qml)
endif()

add_library(AnalyzeView
	ExifParser.cc
	ExifParser.h
	GeoTagController.cc
	GeoTagController.h
	LogDownloadController.cc
	LogDownloadController.h
	MavlinkConsoleController.cc
	MavlinkConsoleController.h
	${MAVLINK_INSPECTOR_SRC}
	PX4LogParser.cc
	PX4LogParser.h
	ULogParser.cc
	ULogParser.h

	${EXTRA_SRC}
)

add_custom_target(AnalyzeViewQml
	SOURCES
		AnalyzePage.qml
		AnalyzeView.qml
		GeoTagPage.qml
		LogDownloadPage.qml
		MavlinkConsolePage.qml
		${MAVLINK_INSPECTOR_QML}
		VibrationPage.qml
)

target_link_libraries(AnalyzeView
	PRIVATE
		qgc

	PUBLIC
		Qt5::Charts
		Qt5::Location
		Qt5::SerialPort
		Qt5::TextToSpeech
		Qt5::Widgets
)

target_include_directories(AnalyzeView INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

