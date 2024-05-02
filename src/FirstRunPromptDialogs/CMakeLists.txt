find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FirstRunPromptDialogs STATIC)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
set(QML_RESOURCES)
qt_add_qml_module(FirstRunPromptDialogs
    URI FirstRunPromptDialogs
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    RESOURCES "${QML_RESOURCES}"
    OUTPUT_TARGETS FirstRunPromptDialogs_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
		QtQuick
		QtQuick.Controls
		QtQuick.Dialogs
		QtQuick.Layouts
		QGroundControl
		QGroundControl.Controls
		QGroundControl.FactControls
		QGroundControl.FactSystem
		QGroundControl.ScreenTools
		QGroundControl.SettingsManager
)
