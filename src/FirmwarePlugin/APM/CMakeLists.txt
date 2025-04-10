if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFirmwarePluginFactory.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFirmwarePluginFactory.h
        CACHE INTERNAL "" FORCE
    )
endif()

if(NOT QGC_DISABLE_APM_PLUGIN)
    # set(APM_RESOURCES)
    # qt_add_resources(APM_RESOURCES APMResources.qrc)

    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFirmwarePlugin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/APMParameterMetaData.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMParameterMetaData.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduCopterFirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduCopterFirmwarePlugin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduPlaneFirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduPlaneFirmwarePlugin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduRoverFirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduRoverFirmwarePlugin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduSubFirmwarePlugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ArduSubFirmwarePlugin.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

if(QGC_DISABLE_APM_MAVLINK)
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QGC_NO_ARDUPILOT_DIALECT
        CACHE INTERNAL "" FORCE
    )
endif()

# CPMAddPackage(
#     NAME ArduPilotParams
#     GITHUB_REPOSITORY ArduPilot/ParameterRepository
#     GIT_TAG main
# )

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# # file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*/*.*)
# qt_add_qml_module(QGC
#     URI QGroundControl.APMFirmwarePlugin
#     VERSION 1.0
#     # RESOURCES ${QML_RESOURCES}
#     QML_FILES ${QML_SOURCES}
#     OUTPUT_TARGETS APMFirmwarePlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.Controls
#         QGroundControl.MultiVehicleManager
#         QGroundControl.ScreenTools
#         QGroundControl.Palette
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         MAVLink
# )
