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

set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.0/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.0.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.1/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.1.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.2/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.2.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.3/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.3.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.4/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.4.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.5/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.5.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Plane-4.6/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.6.xml")

set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.0/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.0.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.1/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.1.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.2/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.2.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.3/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.3.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.4/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.4.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.5/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.5.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Copter-4.6/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.6.xml")

set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.0/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.0.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.1/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.1.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.2/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.2.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.4/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.4.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.5/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.5.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Rover-4.6/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.6.xml")

set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Sub-4.0/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.0.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Sub-4.1/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.1.xml")
set_source_files_properties("${ArduPilotParams_SOURCE_DIR}/Sub-4.5/apm.pdef.xml" PROPERTIES QT_RESOURCE_ALIAS "FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.5.xml")

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_apm_params"
    PREFIX "/"
    FILES
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.0/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.1/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.2/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.3/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.4/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.5/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Plane-4.6/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.0/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.1/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.2/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.3/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.4/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.5/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Copter-4.6/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.0/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.1/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.2/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.4/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.5/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Rover-4.6/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Sub-4.0/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Sub-4.1/apm.pdef.xml"
        "${ArduPilotParams_SOURCE_DIR}/Sub-4.5/apm.pdef.xml"
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
