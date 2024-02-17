qt_add_library(UiPreferencesQmlTest STATIC)

target_link_libraries(QmlControlsTest
	PRIVATE
		qgc
		qgcunittest
)

target_include_directories(QmlControlsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(UiPreferencesQmlTest
    URI uipreferencesqmltest
    VERSION 1.0
    QML_FILES
		DebugWindow.qml
		MockLink.qml
		MockLinkSettings.qml
	IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
