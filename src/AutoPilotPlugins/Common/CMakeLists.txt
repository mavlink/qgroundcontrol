find_package(Qt6 REQUIRED COMPONENTS Core Network Quick)

qt_add_library(CommonAutoPilotPlugin STATIC
    ESP8266Component.cc
    ESP8266Component.h
    ESP8266ComponentController.cc
    ESP8266ComponentController.h
    MotorComponent.cc
    MotorComponent.h
    RadioComponentController.cc
    RadioComponentController.h
    SyslinkComponent.cc
    SyslinkComponent.h
    SyslinkComponentController.cc
    SyslinkComponentController.h
)

target_link_libraries(CommonAutoPilotPlugin
    PRIVATE
        AutoPilotPlugins
        FactSystem
        QGC
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Quick
        FactControls
        VehicleSetup
)

target_include_directories(CommonAutoPilotPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
