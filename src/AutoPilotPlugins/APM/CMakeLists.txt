# ============================================================================
# APM AutoPilot Plugin
# ArduPilot-specific vehicle configuration and setup
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        APMAirframeComponent.cc
        APMAirframeComponent.h
        APMAirframeComponentController.cc
        APMAirframeComponentController.h
        APMAirspeedComponent.cc
        APMAirspeedComponent.h
        APMAutoPilotPlugin.cc
        APMESCComponent.cc
        APMESCComponent.h
        APMAutoPilotPlugin.h
        APMFailsafesComponent.cc
        APMFailsafesComponent.h
        APMFlightModesComponent.cc
        APMFlightModesComponent.h
        APMFlightModesComponentController.cc
        APMFlightModesComponentController.h
        APMFollowComponent.cc
        APMFollowComponent.h
        APMFollowComponentController.cc
        APMFollowComponentController.h
        APMGimbalComponent.cc
        APMGimbalComponent.h
        APMHeliComponent.cc
        APMHeliComponent.h
        APMLightsComponent.cc
        APMLightsComponent.h
        APMMotorComponent.cc
        APMMotorComponent.h
        APMServoComponent.cc
        APMServoComponent.h
        APMPowerComponent.cc
        APMPowerComponent.h
        APMRadioComponent.cc
        APMRadioComponent.h
        APMRemoteSupportComponent.cc
        APMRemoteSupportComponent.h
        APMFlightSafetyComponent.cc
        APMFlightSafetyComponent.h
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
        APMAdvancedTuningCopterComponent.cc
        APMAdvancedTuningCopterComponent.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Code-generation: produce QML config pages from VehicleConfig.json definitions
# ----------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(APM_CONFIG_QML_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/tools/generators/config_qml")
set(APM_CONFIG_PAGES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfig")
set(APM_CONFIG_QML_GEN_DIR       "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Generator source files (changes here trigger regeneration)
file(GLOB _apm_config_generator_sources CONFIGURE_DEPENDS
    "${APM_CONFIG_QML_GENERATOR_DIR}/*.py"
    "${CMAKE_SOURCE_DIR}/tools/generators/common/*.py"
)

# JSON inputs
file(GLOB _apm_config_page_definitions CONFIGURE_DEPENDS "${APM_CONFIG_PAGES_DIR}/*.VehicleConfig.json")

# Generated QML outputs (must match what generate_pages.py produces)
set(_apm_config_generated_qml_names
    APMFailsafesComponent.qml
    APMPowerComponent.qml
    APMFlightSafetyComponent.qml
    APMTuningCopterComponent.qml
)

# Full paths in the build tree
set(_apm_config_generated_qml "")
foreach(_name IN LISTS _apm_config_generated_qml_names)
    list(APPEND _apm_config_generated_qml "${APM_CONFIG_QML_GEN_DIR}/${_name}")
endforeach()

add_custom_command(
    OUTPUT  ${_apm_config_generated_qml}
    COMMAND ${Python3_EXECUTABLE} -m tools.generators.config_qml.generate_pages
            --pages-dir "${APM_CONFIG_PAGES_DIR}"
            --output-dir "${APM_CONFIG_QML_GEN_DIR}"
    DEPENDS ${_apm_config_generator_sources} ${_apm_config_page_definitions}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating APM QML config pages from VehicleConfig.json definitions"
    VERBATIM
)

add_custom_target(GenerateConfigQmlAPM DEPENDS ${_apm_config_generated_qml})

# Set QT_RESOURCE_ALIAS so qt_add_qml_module places generated files at the
# correct QRC path despite living in the build tree.
foreach(_name IN LISTS _apm_config_generated_qml_names)
    set_source_files_properties("${APM_CONFIG_QML_GEN_DIR}/${_name}"
        PROPERTIES QT_RESOURCE_ALIAS "${_name}"
    )
endforeach()

# ----------------------------------------------------------------------------
# APM QML Module
# ----------------------------------------------------------------------------
qt_add_library(AutoPilotPluginsAPMModule STATIC)

# Ensure generated files exist before the module is built
add_dependencies(AutoPilotPluginsAPMModule GenerateConfigQmlAPM)

qt_add_qml_module(AutoPilotPluginsAPMModule
    URI QGroundControl.AutoPilotPlugins.APM
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        # Generated from VehicleConfig.json definitions (output in build tree)
        ${_apm_config_generated_qml}

        # Hand-coded pages and components
        APMAirframeComponent.qml
        APMAirframeComponentSummary.qml
        APMAirspeedComponent.qml
        APMAirspeedComponentSummary.qml
        APMBatteryParams.qml
        APMCalcAmpsPerVoltDialog.qml
        APMCalcVoltageDividerDialog.qml
        APMESCComponent.qml
        APMESCComponentSummary.qml
        APMFailsafesComponentSummary.qml
        APMFailsafesComponentSummarySub.qml
        APMFlightModesComponent.qml
        APMFlightModesComponentSummary.qml
        APMFollowComponent.qml
        APMFollowComponentSummary.qml
        APMGimbalComponent.qml
        APMGimbalInstance.qml
        APMGimbalParams.qml
        APMHeliComponent.qml
        APMLightsComponent.qml
        APMLightsComponentSummary.qml
        APMMotorComponent.qml
        APMServoComponent.qml
        APMNotSupported.qml
        APMPowerComponentSummary.qml
        APMRadioComponentSummary.qml
        APMRemoteSupportComponent.qml
        APMFlightSafetyComponentSub.qml
        APMFlightSafetyComponentSummary.qml
        APMFlightSafetyComponentSummarySub.qml
        APMSensorsComponent.qml
        APMSensorsComponentSummary.qml
        APMSensorIdDecoder.qml
        APMSensorParams.qml
        APMSubFrameComponent.qml
        APMSubFrameComponentSummary.qml
        APMSubMotorComponent.qml
        APMTuningComponentSub.qml
        APMAutoTuneChannelSelector.qml
        APMAdvancedTuningCopterComponent.qml
    RESOURCES
        VehicleConfig/APMFailsafes.VehicleConfig.json
        VehicleConfig/APMPower.VehicleConfig.json
        VehicleConfig/APMFlightSafety.VehicleConfig.json
        VehicleConfig/APMTuningCopter.VehicleConfig.json
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# APM Component Metadata
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_apm_resource
    PREFIX "/json"
    FILES APMFollowComponent.FactMetaData.json
)
