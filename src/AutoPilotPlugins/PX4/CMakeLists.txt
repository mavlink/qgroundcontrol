set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ActuatorComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ActuatorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponentAirframes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponentAirframes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AirframeComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightModesComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightModesComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PowerComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PowerComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PowerComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PowerComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4AirframeLoader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4AirframeLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4AutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4AutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4FlightBehavior.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4FlightBehavior.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4RadioComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4RadioComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4SimpleFlightModesController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4SimpleFlightModesController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4TuningComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4TuningComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SafetyComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SafetyComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorsComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorsComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorsComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorsComponentController.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# qt_add_resources(${CMAKE_PROJECT_NAME} "PX4AutoPilotPlugin_res"
#     PREFIX "/"
#     FILES
#         AirframeFactMetaData.xml
# )

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Images/*.*)
# qt_add_qml_module(${CMAKE_PROJECT_NAME}
#     URI QGroundControl.PX4AutoPilotPlugin
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES ${QML_RESOURCES}
#     OUTPUT_TARGETS PX4AutoPilotPlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         QGroundControl.Controls
#         QGroundControl.ScreenTools
#         QGroundControl.Controllers
#         QGroundControl.Palette
#         QGroundControl.Vehicle
# )
