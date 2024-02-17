qt_add_library(QmlControlsTest STATIC)

target_link_libraries(QmlControlsTest
	PRIVATE
		qgc
		qgcunittest
)

target_include_directories(QmlControlsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(QmlControlsTest
    URI qmlcontrolstest
    VERSION 1.0
    QML_FILES
    	QGCControlDebug.qml
    	QmlTest.qml
	IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)