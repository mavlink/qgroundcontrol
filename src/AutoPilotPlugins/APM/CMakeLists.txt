set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAirframeComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAirframeComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAirframeComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAirframeComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMAutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMCameraComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMCameraComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFlightModesComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFlightModesComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFlightModesComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFlightModesComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFollowComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFollowComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFollowComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMFollowComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMHeliComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMHeliComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMLightsComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMLightsComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMMotorComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMMotorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMPowerComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMPowerComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMRadioComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMRadioComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMRemoteSupportComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMRemoteSupportComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSafetyComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSafetyComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSubFrameComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSubFrameComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSubMotorComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMSubMotorComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/APMTuningComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/APMTuningComponent.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Images/*.*)
# qt_add_qml_module(QGC
#     URI QGroundControl.APMAutoPilotPlugin
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES ${QML_RESOURCES}
#     OUTPUT_TARGETS APMAutoPilotPlugin_targets
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
