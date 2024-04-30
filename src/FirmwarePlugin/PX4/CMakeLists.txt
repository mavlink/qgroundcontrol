find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(PX4FirmwarePlugin STATIC
    px4_custom_mode.h
    PX4FirmwarePlugin.cc
    PX4FirmwarePlugin.h
    PX4FirmwarePluginFactory.cc
    PX4FirmwarePluginFactory.h
    PX4ParameterMetaData.cc
    PX4ParameterMetaData.h
    # PX4Resources.qrc
)

target_link_libraries(PX4FirmwarePlugin
    PRIVATE
        AutoPilotPlugins
        MissionManager
        PX4AutoPilotPlugin
        qgc
        Settings
        VehicleSetup
        Utilities
    PUBLIC
        Qt6::Core
        comm
        FactSystem
        FirmwarePlugin
)

target_include_directories(PX4FirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(PX4_RESOURCES)
qt_add_resources(PX4_RESOURCES PX4Resources.qrc)
target_sources(PX4FirmwarePlugin PUBLIC ${PX4_RESOURCES})
