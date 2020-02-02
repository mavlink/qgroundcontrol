
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		LogDownloadTest.cc
	)
endif()

add_library(AnalyzeView
	ExifParser.cc
	GeoTagController.cc
	MAVLinkInspectorController.cc
	LogDownloadController.cc
	MavlinkConsoleController.cc
	PX4LogParser.cc
	ULogParser.cc
	${EXTRA_SRC}
)

add_custom_target(AnalyzeViewQml
	SOURCES
		AnalyzePage.qml
		AnalyzeView.qml
		GeoTagPage.qml
		LogDownloadPage.qml
		MavlinkConsolePage.qml
		MAVLinkInspectorPage.qml
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

