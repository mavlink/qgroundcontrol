target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(AutoPilotPluginsCommonModule STATIC)

qt_add_qml_module(AutoPilotPluginsCommonModule
    URI QGroundControl.AutoPilotPlugins.Common
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ESP8266Component.qml
        ESP8266ComponentSummary.qml
        MotorComponent.qml
        RadioComponent.qml
        SyslinkComponent.qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)
