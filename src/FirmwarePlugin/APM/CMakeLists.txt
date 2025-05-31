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

if(QGC_DISABLE_APM_MAVLINK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_ARDUPILOT_DIALECT)
endif()

# CPMAddPackage(
#     NAME ArduPilotParams
#     GITHUB_REPOSITORY ArduPilot/ParameterRepository
#     GIT_TAG main
# )
