find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FlightMapItems STATIC)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
set(QML_RESOURCES)
qt_add_qml_module(FlightMapItems
    URI FlightMapItems
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    RESOURCES "${QML_RESOURCES}"
    OUTPUT_TARGETS FlightMapItems_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
		QtCharts
    	QtLocation
    	QtMultimedia
		QtPositioning
	    QtQml
		QtQml.Models
		QtQuick
		QtQuick.Controls
		QtQuick.Dialogs
		QtQuick.Extras
		QtQuick.Layouts
		QtQuick.Window
		QGroundControl
		QGroundControl.Controllers
		QGroundControl.Controls
		QGroundControl.FactControls
		QGroundControl.FactSystem
		QGroundControl.FlightDisplay
		QGroundControl.FlightMap
		QGroundControl.Palette
		QGroundControl.ScreenTools
		QGroundControl.ShapeFileHelper
		QGroundControl.UTMSP
		QGroundControl.Vehicle
)
