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

# Add JSON files
file(GLOB_RECURSE JSON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_firmaware_plugin_apm
    PREFIX "/json"
    FILES
    ${JSON_FILES}
)

# Add other resource
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
    GIT_TAG main
)

# Dynamically discover all parameter definition files.
# Directory layout: {Vehicle}-{Major}.{Minor}/apm.pdef.xml
# Resource alias:   FirmwarePlugin/APM/APMParameterFactMetaData.{Vehicle}.{Major}.{Minor}.xml
file(GLOB _apm_pdef_files "${ArduPilotParams_SOURCE_DIR}/*/apm.pdef.xml")
list(SORT _apm_pdef_files)

set(_apm_param_resources "")
foreach(_pdef IN LISTS _apm_pdef_files)
    get_filename_component(_dir "${_pdef}" DIRECTORY)
    get_filename_component(_dirname "${_dir}" NAME)

    # Parse "{Vehicle}-{Version}" directory name (e.g. "Plane-4.7")
    string(REGEX MATCH "^([A-Za-z_]+)-(.+)$" _match "${_dirname}")
    if(NOT _match)
        continue()
    endif()
    set(_vehicle "${CMAKE_MATCH_1}")
    set(_version "${CMAKE_MATCH_2}")

    # Convert dash-separated version to dot-separated (e.g. "4.7" stays, "3.10" stays)
    string(REPLACE "-" "." _version_dots "${_version}")

    set(_alias "FirmwarePlugin/APM/APMParameterFactMetaData.${_vehicle}.${_version_dots}.xml")
    set_source_files_properties("${_pdef}" PROPERTIES QT_RESOURCE_ALIAS "${_alias}")
    list(APPEND _apm_param_resources "${_pdef}")
endforeach()

list(LENGTH _apm_param_resources _apm_param_count)
message(STATUS "APM: Found ${_apm_param_count} parameter definition files")

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_apm_params"
    PREFIX "/"
    FILES ${_apm_param_resources}
)

# Add qml module
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
