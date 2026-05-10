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

# Always track HEAD of the ParameterRepository.
# CPM cannot do this (it caches by tag string and never re-fetches a branch).
# Instead, clone on first configure and pull on every subsequent configure so
# the embedded metadata is always up to date with new firmware versions and
# updated parameter descriptions.
# Store alongside other CPM packages so the clone survives build-dir wipes and
# is shared across Debug/Release/etc. build trees.
find_package(Git REQUIRED)
set(_ardupilot_params_dir "${CPM_SOURCE_CACHE}/ardupilotparams/HEAD")
if(NOT EXISTS "${_ardupilot_params_dir}/.git")
    message(STATUS "APM: Cloning ArduPilot/ParameterRepository...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone --depth=1 --progress
            https://github.com/ArduPilot/ParameterRepository.git
            "${_ardupilot_params_dir}"
        RESULT_VARIABLE _git_result
        ERROR_VARIABLE _git_error
        OUTPUT_QUIET
    )
    if(NOT _git_result EQUAL 0)
        message(FATAL_ERROR "APM: Failed to clone ArduPilot/ParameterRepository (exit code ${_git_result}):\n${_git_error}")
    endif()
else()
    message(STATUS "APM: Updating ArduPilot/ParameterRepository...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} fetch --depth=1 origin
        WORKING_DIRECTORY "${_ardupilot_params_dir}"
        RESULT_VARIABLE _git_result
        ERROR_VARIABLE _git_error
        OUTPUT_QUIET
    )
    if(_git_result EQUAL 0)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} reset --hard origin/HEAD
            WORKING_DIRECTORY "${_ardupilot_params_dir}"
            OUTPUT_QUIET ERROR_QUIET
        )
    else()
        message(WARNING "APM: git fetch failed (exit code ${_git_result}) — using cached metadata:\n${_git_error}")
    endif()
endif()
set(ArduPilotParams_SOURCE_DIR "${_ardupilot_params_dir}")

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
