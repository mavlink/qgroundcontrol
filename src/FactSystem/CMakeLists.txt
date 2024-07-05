add_subdirectory(FactControls)

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FactSystem STATIC
    Fact.cc
    Fact.h
    FactGroup.cc
    FactGroup.h
    FactMetaData.cc
    FactMetaData.h
    FactValueSliderListModel.cc
    FactValueSliderListModel.h
    ParameterManager.cc
    ParameterManager.h
    SettingsFact.cc
    SettingsFact.h
)

target_link_libraries(FactSystem
    PRIVATE
        Qt6::Qml
        API
        AutoPilotPlugins
        FactControls
        FirmwarePlugin
        QGC
        Settings
        Utilities
        Vehicle
        VehicleComponents
    PUBLIC
        Qt6::Core
        MAVLink
)

target_include_directories(FactSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
