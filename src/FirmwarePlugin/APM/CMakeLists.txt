if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APMFirmwarePluginFactory.cc
            APMFirmwarePluginFactory.h
    )
endif()

if(NOT QGC_DISABLE_APM_PLUGIN)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APM.h
            APMFirmwarePlugin.cc
            APMFirmwarePlugin.h
            APMParameterMetaData.cc
            APMParameterMetaData.h
            ArduCopterFirmwarePlugin.cc
            ArduCopterFirmwarePlugin.h
            ArduPlaneFirmwarePlugin.cc
            ArduPlaneFirmwarePlugin.h
            ArduRoverFirmwarePlugin.cc
            ArduRoverFirmwarePlugin.h
            ArduSubFirmwarePlugin.cc
            ArduSubFirmwarePlugin.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE JSON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_firmaware_plugin_apm
    PREFIX "/json"
    FILES
    ${JSON_FILES}
)

qt_add_resources(${CMAKE_PROJECT_NAME} firmaware_plugin_apm_resource
    PREFIX "/FirmwarePlugin/APM"
    FILES
        Copter.OfflineEditing.params
        Plane.OfflineEditing.params
        Rover.OfflineEditing.params
        Sub.OfflineEditing.params
)

#===========================================================================#

CPMAddPackage(
    NAME ArduPilotParams
    GITHUB_REPOSITORY ArduPilot/ParameterRepository
    GIT_TAG 037ff6bcfc080608544a4526f2c22406f2cce60f
)

# Filters applied to discovered ArduPilot parameter dirs (e.g. "Copter-4.7").
# Regex (CMake syntax) — match against the dir name; matches are dropped.
set(QGC_APM_PARAMS_EXCLUDE
    "AP_Periph-.*"
    "Blimp-.*"
    "^Copter-3[.]"
    "^Plane-3[.]"
    "^Rover-3[.]"
    CACHE STRING
    "Regex patterns matched against ArduPilot param dir names to exclude from the embedded resource")

# Dynamically discover all parameter definition files.
# Directory layout: {Vehicle}-{Major}.{Minor}/apm.pdef.json
# Resource alias:   FirmwarePlugin/APM/APMParameterFactMetaData.{Vehicle}.{Major}.{Minor}.json
file(GLOB _apm_pdef_files "${ArduPilotParams_SOURCE_DIR}/*/apm.pdef.json")
list(SORT _apm_pdef_files)

set(_apm_param_resources "")
set(_apm_param_skipped "")
foreach(_pdef IN LISTS _apm_pdef_files)
    get_filename_component(_dir "${_pdef}" DIRECTORY)
    get_filename_component(_dirname "${_dir}" NAME)

    string(REGEX MATCH "^([A-Za-z_]+)-(.+)$" _match "${_dirname}")
    if(NOT _match)
        continue()
    endif()
    set(_vehicle "${CMAKE_MATCH_1}")
    set(_version "${CMAKE_MATCH_2}")

    set(_excluded FALSE)
    foreach(_pattern IN LISTS QGC_APM_PARAMS_EXCLUDE)
        if(_pattern AND "${_dirname}" MATCHES "${_pattern}")
            set(_excluded TRUE)
            break()
        endif()
    endforeach()
    if(_excluded)
        list(APPEND _apm_param_skipped "${_dirname}")
        continue()
    endif()

    string(REPLACE "-" "." _version_dots "${_version}")

    set(_alias "FirmwarePlugin/APM/APMParameterFactMetaData.${_vehicle}.${_version_dots}.json")
    set_source_files_properties("${_pdef}" PROPERTIES QT_RESOURCE_ALIAS "${_alias}")
    list(APPEND _apm_param_resources "${_pdef}")
endforeach()

list(LENGTH _apm_param_resources _apm_param_count)
message(STATUS "APM: Found ${_apm_param_count} parameter definition files")
if(_apm_param_skipped)
    list(LENGTH _apm_param_skipped _apm_param_skipped_count)
    message(STATUS "APM: Excluded ${_apm_param_skipped_count} parameter dirs by QGC_APM_PARAMS_EXCLUDE: ${_apm_param_skipped}")
endif()

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_apm_params"
    PREFIX "/"
    FILES ${_apm_param_resources}
)

qt_add_library(APMFirmwareModule STATIC)

qt_add_qml_module(APMFirmwareModule
    URI QGroundControl.FirmwarePlugin.APM
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        APMBatteryIndicator.qml
        APMFlightModeIndicator.qml
        APMMainStatusIndicator.qml
        APMSupportForwardingIndicator.qml
    NO_PLUGIN
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE APMFirmwareModule)
