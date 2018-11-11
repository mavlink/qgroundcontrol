
add_library(QmlControls
	AppMessages.cc
	CoordinateVector.cc
	EditPositionDialogController.cc
	ParameterEditorController.cc
	QGCFileDialogController.cc
	QGCGeoBoundingCube.cc
	QGCImageProvider.cc
	QGroundControlQmlGlobal.cc
	QmlObjectListModel.cc
	QmlTestWidget.cc
	RCChannelMonitorController.cc
	ScreenToolsController.cc
)

target_link_libraries(QmlControls
        PUBLIC
		qgc

		FactSystem
	PUBLIC
		Qt5::Concurrent
		Qt5::Location
		Qt5::SerialPort
		Qt5::TextToSpeech
		Qt5::Widgets
)

target_include_directories(QmlControls INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

