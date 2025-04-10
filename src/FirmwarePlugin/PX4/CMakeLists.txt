if(NOT QGC_DISABLE_PX4_PLUGIN_FACTORY)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwarePluginFactory.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwarePluginFactory.h
        CACHE INTERNAL "" FORCE
    )
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN)
    # set(PX4_RESOURCES)
    # qt_add_resources(PX4_RESOURCES PX4Resources.qrc)

    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/px4_custom_mode.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwarePlugin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4ParameterMetaData.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4ParameterMetaData.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

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
