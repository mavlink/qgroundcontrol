find_package(Qt6 REQUIRED COMPONENTS Core Network)

if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
    target_sources(QGC
        PRIVATE
            APMFirmwarePluginFactory.cc
            APMFirmwarePluginFactory.h
    )
endif()

if(NOT QGC_DISABLE_APM_PLUGIN)
    # set(APM_RESOURCES)
    # qt_add_resources(APM_RESOURCES APMResources.qrc)

    target_sources(QGC
        PRIVATE
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
            # ${APM_RESOURCES}
    )
endif()

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Network
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# include(CPM)
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
