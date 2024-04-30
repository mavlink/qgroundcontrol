find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_add_library(APMAutoPilotPlugin STATIC
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

target_link_libraries(APMAutoPilotPlugin
    PRIVATE
        Actuators
        APMFirmwarePlugin
        AutoPilotPlugins
        FactSystem
        qgc
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::Quick
        CommonAutoPilotPlugin
        FactControls
        VehicleSetup
)

target_include_directories(APMAutoPilotPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
