if(NOT QGC_DISABLE_PX4_PLUGIN_FACTORY)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            PX4FirmwarePluginFactory.cc
            PX4FirmwarePluginFactory.h
    )
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN)
    # set(PX4_RESOURCES)
    # qt_add_resources(PX4_RESOURCES PX4Resources.qrc)

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            px4_custom_mode.h
            PX4FirmwarePlugin.cc
            PX4FirmwarePlugin.h
            PX4ParameterMetaData.cc
            PX4ParameterMetaData.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# # file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*/*.*)
# qt_add_qml_module(QGC
#     URI QGroundControl.PX4FirmwarePlugin
#     VERSION 1.0
#     # RESOURCES ${QML_RESOURCES}
#     QML_FILES ${QML_SOURCES}
#     OUTPUT_TARGETS PX4FirmwarePlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.Controls
#         QGroundControl.MultiVehicleManager
#         QGroundControl.ScreenTools
#         QGroundControl.Palette
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
# )
