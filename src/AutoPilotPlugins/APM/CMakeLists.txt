find_package(Qt6 REQUIRED COMPONENTS Core Quick)

target_sources(QGC
    PRIVATE
        APMAirframeComponent.cc
        APMAirframeComponent.h
        APMAirframeComponentController.cc
        APMAirframeComponentController.h
        APMAutoPilotPlugin.cc
        APMAutoPilotPlugin.h
        APMCameraComponent.cc
        APMCameraComponent.h
        APMFlightModesComponent.cc
        APMFlightModesComponent.h
        APMFlightModesComponentController.cc
        APMFlightModesComponentController.h
        APMFollowComponent.cc
        APMFollowComponent.h
        APMFollowComponentController.cc
        APMFollowComponentController.h
        APMHeliComponent.cc
        APMHeliComponent.h
        APMLightsComponent.cc
        APMLightsComponent.h
        APMMotorComponent.cc
        APMMotorComponent.h
        APMPowerComponent.cc
        APMPowerComponent.h
        APMRadioComponent.cc
        APMRadioComponent.h
        APMRemoteSupportComponent.cc
        APMRemoteSupportComponent.h
        APMSafetyComponent.cc
        APMSafetyComponent.h
        APMSensorsComponent.cc
        APMSensorsComponent.h
        APMSensorsComponentController.cc
        APMSensorsComponentController.h
        APMSubFrameComponent.cc
        APMSubFrameComponent.h
        APMSubMotorComponentController.cc
        APMSubMotorComponentController.h
        APMTuningComponent.cc
        APMTuningComponent.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Quick
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
