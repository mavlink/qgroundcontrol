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

CPMAddPackage(
    NAME ArduPilotParams
    GITHUB_REPOSITORY ArduPilot/ParameterRepository
    GIT_TAG main
)

set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.0/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.0.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.1/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.1.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.2/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.2.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.3/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.3.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.4/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.4.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.5/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.5.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Plane-4.6/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.6.xml)

set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.0/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.0.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.1/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.1.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.2/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.2.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.3/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.3.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.4/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.4.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.5/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.5.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Copter-4.6/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.6.xml)

set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.0/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.0.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.1/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.1.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.2/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.2.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.4/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.4.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.5/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.5.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Rover-4.6/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.6.xml)

set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Sub-4.0/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.0.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Sub-4.1/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.1.xml)
set_source_files_properties(${ArduPilotParams_SOURCE_DIR}/Sub-4.5/apm.pdef.xml TARGET_DIRECTORY ${CMAKE_PROJECT_NAME} PROPERTIES QT_RESOURCE_ALIAS FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.5.xml)

set(QGC_ARDUPILOT_PARAMS_RESOURCES
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
    CACHE STRING "ArduPilot Parameter Resources"
)
