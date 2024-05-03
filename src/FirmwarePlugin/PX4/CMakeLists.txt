find_package(Qt6 REQUIRED COMPONENTS Core)

set(PX4_RESOURCES)
qt_add_resources(PX4_RESOURCES PX4Resources.qrc)
qt_add_library(PX4FirmwarePlugin STATIC
    px4_custom_mode.h
    PX4FirmwarePlugin.cc
    PX4FirmwarePlugin.h
    # PX4FirmwarePluginFactory.cc
    # PX4FirmwarePluginFactory.h
    PX4ParameterMetaData.cc
    PX4ParameterMetaData.h
    ${PX4_RESOURCES}
)

target_link_libraries(PX4FirmwarePlugin
    PRIVATE
        AutoPilotPlugins
        MissionManager
        PX4AutoPilotPlugin
        QGC
        Settings
        VehicleSetup
        Utilities
    PUBLIC
        Qt6::Core
        Comms
        FactSystem
        FirmwarePlugin
)

target_include_directories(PX4FirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# # file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*/*.*)
# qt_add_qml_module(PX4FirmwarePlugin
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
