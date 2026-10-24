# ============================================================================
# PX4 AutoPilot Plugin
# PX4-specific vehicle configuration and setup
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ActuatorComponent.cc
        ActuatorComponent.h
        AirframeComponent.cc
        AirframeComponent.h
        AirframeComponentAirframes.cc
        AirframeComponentAirframes.h
        AirframeComponentController.cc
        AirframeComponentController.h
        FailureInjection.cc
        FailureInjection.h
        FailureInjectionComponent.cc
        FailureInjectionComponent.h
        FlightModesComponent.cc
        FlightModesComponent.h
        PX4AirframeLoader.cc
        PX4AirframeLoader.h
        PX4AutoPilotPlugin.cc
        PX4AutoPilotPlugin.h
        PX4BatteryComponent.cc
        PX4BatteryComponent.h
        PX4BatteryController.cc
        PX4BatteryController.h
        PX4CollisionComponent.cc
        PX4CollisionComponent.h
        PX4EscComponent.cc
        PX4EscComponent.h
        PX4EscController.cc
        PX4EscController.h
        PX4FailsafesComponent.cc
        PX4FailsafesComponent.h
        PX4FlightBehavior.cc
        PX4FlightBehavior.h
        PX4FlightModesController.cc
        PX4FlightModesController.h
        PX4RadioComponent.cc
        PX4RadioComponent.h
        PX4ReturnLandComponent.cc
        PX4ReturnLandComponent.h
        PX4SerialPortConfig.cc
        PX4SerialPortConfig.h
        PX4SerialPortsComponent.cc
        PX4SerialPortsComponent.h
        PX4SerialPortsController.cc
        PX4SerialPortsController.h
        PX4TuningComponent.cc
        PX4TuningComponent.h
        SensorsComponent.cc
        SensorsComponent.h
        SensorsComponentController.cc
        SensorsComponentController.h
)

# ----------------------------------------------------------------------------
# PX4 Airframe Metadata
# ----------------------------------------------------------------------------
qt_add_resources(
    ${CMAKE_PROJECT_NAME} autopilot_plugin_px4_resource
    PREFIX "/AutoPilotPlugins/PX4"
    FILES
        AirframeFactMetaData.xml
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# PX4 QML Module
# ----------------------------------------------------------------------------
qt_add_library(AutoPilotPluginsPX4Module STATIC)

qt_add_qml_module(
    AutoPilotPluginsPX4Module
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
        BatteryThresholdBar.qml
        CalcAmpsPerVoltDialog.qml
        CalcVoltageDividerDialog.qml
        ConfigChoiceComboBox.qml
        ConfigGroupHeader.qml
        ConfigNote.qml
        ConfigParamCell.qml
        ConfigRow.qml
        ConfigRowGroups.qml
        ConfigRowsFilterBar.qml
        ConfigRowsPanel.qml
        ESCCalibrationDialog.qml
        FailureInjectionComponent.qml
        FailureInjectionInstances.js
        FlightModesComponentSummary.qml
        PX4BatteryComponent.qml
        PX4BatteryComponentSummary.qml
        PX4BatteryRows.js
        PX4CollisionComponent.qml
        PX4CollisionComponentSummary.qml
        PX4CollisionRows.js
        PX4ConfigRows.js
        PX4EscComponent.qml
        PX4EscRows.js
        PX4FailsafeRows.js
        PX4FailsafesComponent.qml
        PX4FlightBehaviorCopter.qml
        PX4FlightModeRows.js
        PX4FlightModes.qml
        PX4RadioComponentSummary.qml
        PX4ReturnLandComponent.qml
        PX4ReturnLandComponentSummary.qml
        PX4ReturnLandRows.js
        PX4SerialPortsComponent.qml
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
        PX4TuningComponentSpacecraft.qml
        PX4TuningComponentSpacecraftAll.qml
        PX4TuningComponentSpacecraftAttitude.qml
        PX4TuningComponentSpacecraftPosition.qml
        PX4TuningComponentSpacecraftRate.qml
        PX4TuningComponentSpacecraftVelocity.qml
        PX4TuningComponentVTOL.qml
        SensorsComponent.qml
        SensorsComponentSummary.qml
        SensorsComponentSummaryFixedWing.qml
        SensorsSetup.qml
    NO_PLUGIN
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME} autopilot_plugin_px4_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/CameraTrigger.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/HITL.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/LandMode.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/no-logging.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/no-logging-light.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/ReturnToHomeAltitude.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleDown.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleDownRotate.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleLeft.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleLeftRotate.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleNoseDown.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleNoseDownRotate.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleRight.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleRightRotate.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleTailDown.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleTailDownRotate.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleUpsideDown.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/VehicleUpsideDownRotate.png"
)
