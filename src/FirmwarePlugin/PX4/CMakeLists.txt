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
