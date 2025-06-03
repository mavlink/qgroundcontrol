target_sources(${CMAKE_PROJECT_NAME}
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(AutoPilotPluginsAPMModule STATIC)

qt_add_qml_module(AutoPilotPluginsAPMModule
    URI QGroundControl.AutoPilotPlugins.APM
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        APMAirframeComponent.qml
        APMAirframeComponentSummary.qml
        APMCameraComponent.qml
        APMCameraComponentSummary.qml
        APMCameraSubComponent.qml
        APMFlightModesComponent.qml
        APMFlightModesComponentSummary.qml
        APMFollowComponent.qml
        APMFollowComponentSummary.qml
        APMHeliComponent.qml
        APMLightsComponent.qml
        APMLightsComponentSummary.qml
        APMMotorComponent.qml
        APMNotSupported.qml
        APMPowerComponent.qml
        APMPowerComponentSummary.qml
        APMRadioComponentSummary.qml
        APMRemoteSupportComponent.qml
        APMSafetyComponent.qml
        APMSafetyComponentCopter.qml
        APMSafetyComponentPlane.qml
        APMSafetyComponentRover.qml
        APMSafetyComponentSub.qml
        APMSafetyComponentSummary.qml
        APMSafetyComponentSummaryCopter.qml
        APMSafetyComponentSummaryPlane.qml
        APMSafetyComponentSummaryRover.qml
        APMSafetyComponentSummarySub.qml
        APMSensorsComponent.qml
        APMSensorsComponentSummary.qml
        APMSensorIdDecoder.qml
        APMSensorParams.qml
        APMSubFrameComponent.qml
        APMSubFrameComponentSummary.qml
        APMSubMotorComponent.qml
        APMTuningComponentCopter.qml
        APMTuningComponentSub.qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)
