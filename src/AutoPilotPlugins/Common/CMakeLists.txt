# ============================================================================
# Common AutoPilot Plugins
# Components shared across different autopilot types
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ESP8266Component.cc
        ESP8266Component.h
        ESP8266ComponentController.cc
        ESP8266ComponentController.h
        ESP8266StateMachine.cc
        ESP8266StateMachine.h
        ScriptingComponent.cc
        ScriptingComponent.h
        MotorComponent.cc
        MotorComponent.h
        JoystickComponent.cc
        JoystickComponent.h
        RadioComponentController.cc
        RadioComponentController.h
        SyslinkComponent.cc
        SyslinkComponent.h
        SyslinkComponentController.cc
        SyslinkComponentController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Common Sensors Component
# ----------------------------------------------------------------------------
add_subdirectory(Sensors)

# ----------------------------------------------------------------------------
# Common AutoPilot Plugins QML Module
# ----------------------------------------------------------------------------
qt_add_library(AutoPilotPluginsCommonModule STATIC)

qt_add_qml_module(AutoPilotPluginsCommonModule
    URI QGroundControl.AutoPilotPlugins.Common
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ESP8266Component.qml
        ESP8266ComponentSummary.qml
        ScriptingComponent.qml
        JoystickComponentSummary.qml
        MotorComponent.qml
        RadioComponent.qml
        SyslinkComponent.qml
    NO_PLUGIN
)
