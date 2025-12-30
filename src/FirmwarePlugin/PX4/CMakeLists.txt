if(NOT QGC_DISABLE_PX4_PLUGIN_FACTORY)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            PX4FirmwarePluginFactory.cc
            PX4FirmwarePluginFactory.h
    )
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            px4_custom_mode.h
            PX4FirmwarePlugin.cc
            PX4FirmwarePlugin.h
            PX4ParameterMetaData.cc
            PX4ParameterMetaData.h
    )
endif()

# Add JSON files
file(GLOB_RECURSE JSON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_firmaware_plugin_px4
    PREFIX "/json"
    FILES
    ${JSON_FILES}
)

# Add other resource
set_source_files_properties(V1.4.OfflineEditing.params PROPERTIES QT_RESOURCE_ALIAS PX4.OfflineEditing.params)

qt_add_resources(${CMAKE_PROJECT_NAME} firmaware_plugin_px4_resource
    PREFIX "/FirmwarePlugin/PX4"
    FILES
        PX4ParameterFactMetaData.xml
        V1.4.OfflineEditing.params
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add qml module
qt_add_library(PX4FirmwareModule STATIC)

qt_add_qml_module(PX4FirmwareModule
    URI QGroundControl.FirmwarePlugin.PX4
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        PX4BatteryIndicator.qml
        PX4FlightModeIndicator.qml
        PX4MainStatusIndicator.qml
    NO_PLUGIN
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PX4FirmwareModule)
