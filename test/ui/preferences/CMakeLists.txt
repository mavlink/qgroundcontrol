find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(UiPreferencesQmlTest STATIC)

qt_add_qml_module(UiPreferencesQmlTest
    URI uipreferencesqmltest
    VERSION 1.0
    QML_FILES
		DebugWindow.qml
		MockLink.qml
		MockLinkSettings.qml
	IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
