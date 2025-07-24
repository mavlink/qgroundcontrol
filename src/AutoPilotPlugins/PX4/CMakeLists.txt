target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ActuatorComponent.cc
        ActuatorComponent.h
        AirframeComponent.cc
        AirframeComponent.h
        AirframeComponentAirframes.cc
        AirframeComponentAirframes.h
        AirframeComponentController.cc
        AirframeComponentController.h
        FlightModesComponent.cc
        FlightModesComponent.h
        PowerComponent.cc
        PowerComponent.h
        PowerComponentController.cc
        PowerComponentController.h
        PX4AirframeLoader.cc
        PX4AirframeLoader.h
        PX4AutoPilotPlugin.cc
        PX4AutoPilotPlugin.h
        PX4FlightBehavior.cc
        PX4FlightBehavior.h
        PX4RadioComponent.cc
        PX4RadioComponent.h
        PX4SimpleFlightModesController.cc
        PX4SimpleFlightModesController.h
        PX4TuningComponent.cc
        PX4TuningComponent.h
        SafetyComponent.cc
        SafetyComponent.h
        SensorsComponent.cc
        SensorsComponent.h
        SensorsComponentController.cc
        SensorsComponentController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(AutoPilotPluginsPX4Module STATIC)

qt_add_qml_module(AutoPilotPluginsPX4Module
    URI QGroundControl.AutoPilotPlugins.PX4
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ActuatorComponent.qml
        ActuatorFact.qml
        ActuatorSlider.qml
        AirframeComponent.qml
        AirframeComponentSummary.qml
        BatteryParams.qml
        FlightModesComponentSummary.qml
        PowerComponent.qml
        PowerComponentSummary.qml
        PX4FlightBehaviorCopter.qml
        PX4FlightModes.qml
        PX4RadioComponentSummary.qml
        PX4SimpleFlightModes.qml
        PX4TuningComponent.qml
        PX4TuningComponentCopter.qml
        PX4TuningComponentCopterAll.qml
        PX4TuningComponentCopterAttitude.qml
        PX4TuningComponentCopterPosition.qml
        PX4TuningComponentCopterRate.qml
        PX4TuningComponentCopterVelocity.qml
        PX4TuningComponentPlane.qml
        PX4TuningComponentPlaneAll.qml
        PX4TuningComponentPlaneAttitude.qml
        PX4TuningComponentPlaneRate.qml
        PX4TuningComponentPlaneTECS.qml
        PX4TuningComponentVTOL.qml
        SafetyComponent.qml
        SafetyComponentSummary.qml
        SensorsComponent.qml
        SensorsComponentSummary.qml
        SensorsComponentSummaryFixedWing.qml
        SensorsSetup.qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)
