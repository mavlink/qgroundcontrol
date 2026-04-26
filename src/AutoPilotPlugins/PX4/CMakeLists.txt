# ============================================================================
# PX4 AutoPilot Plugin
# PX4-specific vehicle configuration and setup
# ============================================================================

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

# ----------------------------------------------------------------------------
# PX4 Airframe Metadata
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_px4_resource
    PREFIX "/AutoPilotPlugins/PX4"
    FILES AirframeFactMetaData.xml
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Code-generation: produce QML config pages from VehicleConfig.json definitions
# ----------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CONFIG_QML_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/tools/generators/config_qml")
set(CONFIG_PAGES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfig")
set(CONFIG_QML_GEN_DIR       "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Generator source files (changes here trigger regeneration)
file(GLOB _config_generator_sources CONFIGURE_DEPENDS
    "${CONFIG_QML_GENERATOR_DIR}/*.py"
    "${CMAKE_SOURCE_DIR}/tools/generators/common/*.py"
)

# JSON inputs
file(GLOB _config_page_definitions CONFIGURE_DEPENDS "${CONFIG_PAGES_DIR}/*.VehicleConfig.json")

# Generated QML outputs (must match what generate_pages.py produces)
set(_config_generated_qml_names
    PowerComponent.qml
    SafetyComponent.qml
)

# Full paths in the build tree
set(_config_generated_qml "")
foreach(_name IN LISTS _config_generated_qml_names)
    list(APPEND _config_generated_qml "${CONFIG_QML_GEN_DIR}/${_name}")
endforeach()

add_custom_command(
    OUTPUT  ${_config_generated_qml}
    COMMAND ${Python3_EXECUTABLE} -m tools.generators.config_qml.generate_pages
            --pages-dir "${CONFIG_PAGES_DIR}"
            --output-dir "${CONFIG_QML_GEN_DIR}"
    DEPENDS ${_config_generator_sources} ${_config_page_definitions}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating QML config pages from VehicleConfig.json definitions"
    VERBATIM
)

add_custom_target(GenerateConfigQmlPX4 DEPENDS ${_config_generated_qml})

# Set QT_RESOURCE_ALIAS so qt_add_qml_module places generated files at the
# correct QRC path despite living in the build tree.
foreach(_name IN LISTS _config_generated_qml_names)
    set_source_files_properties("${CONFIG_QML_GEN_DIR}/${_name}"
        PROPERTIES QT_RESOURCE_ALIAS "${_name}"
    )
endforeach()

# ----------------------------------------------------------------------------
# PX4 QML Module
# ----------------------------------------------------------------------------
qt_add_library(AutoPilotPluginsPX4Module STATIC)

# Ensure generated files exist before the module is built
add_dependencies(AutoPilotPluginsPX4Module GenerateConfigQmlPX4)

qt_add_qml_module(AutoPilotPluginsPX4Module
    URI QGroundControl.AutoPilotPlugins.PX4
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        # Generated from VehicleConfig.json definitions (output in build tree)
        ${_config_generated_qml}

        # Hand-coded pages and components
        ActuatorComponent.qml
        ActuatorFact.qml
        ActuatorSlider.qml
        AirframeComponent.qml
        AirframeComponentSummary.qml
        BatteryParams.qml
        CalcAmpsPerVoltDialog.qml
        CalcVoltageDividerDialog.qml
        ESCCalibrationDialog.qml
        FlightModesComponentSummary.qml
        PowerComponentSummary.qml
        PX4FlightBehaviorCopter.qml
        PX4FlightModes.qml
        PX4RadioComponentSummary.qml
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
        SafetyComponentSummary.qml
        SensorsComponent.qml
        SensorsComponentSummary.qml
        SensorsComponentSummaryFixedWing.qml
        SensorsSetup.qml
    RESOURCES
        VehicleConfig/Power.VehicleConfig.json
        VehicleConfig/Safety.VehicleConfig.json
    NO_PLUGIN
)

qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_px4_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/CameraTrigger.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/DatalinkLoss.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/DatalinkLossLight.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/GeoFence.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/GeoFenceLight.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/HITL.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/LandMode.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/LandModeCopter.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/LowBattery.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/LowBatteryLight.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/no-logging.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/no-logging-light.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/ObjectAvoidance.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_01cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_02cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_03cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_04cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_05cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/PowerComponentBattery_06cell.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/RCLoss.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/RCLossLight.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/ReturnToHomeAltitude.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/PX4/Images/ReturnToHomeAltitudeCopter.svg"
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
