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
        ScriptingComponent.cc
        ScriptingComponent.h
        MotorComponent.cc
        MotorComponent.h
        JoystickComponent.cc
        JoystickComponent.h
        PowerModulePresetController.cc
        PowerModulePresetController.h
        RadioComponentController.cc
        RadioComponentController.h
        SyslinkComponent.cc
        SyslinkComponent.h
        SyslinkComponentController.cc
        SyslinkComponentController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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

# ----------------------------------------------------------------------------
# Common AutoPilot Plugins Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_common_resource
    PREFIX "/json"
    FILES PowerModulePresets.json
)

qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_common_images
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/Download.svg"
        "${CMAKE_SOURCE_DIR}/resources/Upload.svg"
)

# Airframe icons. URL form: /qmlimages/Airframe/<name>.svg, populated dynamically
# from PX4 AirframeFactMetaData.xml image="..." values and APM frame-class lookups.
qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_common_airframes
    PREFIX "/qmlimages/Airframe"
    BASE "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/AirframeSimulation.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/AirframeUnknown.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Airship.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Autogyro.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Boat.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/FlyingWing.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/FreeFlyer.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Helicopter.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/HelicopterCoaxial.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/HexaRotorPlus.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/HexaRotorX.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/OctoRotorPlus.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/OctoRotorPlusCoaxial.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/OctoRotorX.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/OctoRotorXCoaxial.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Plane.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/PlaneATail.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/PlaneVTail.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorATail.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorH.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorPlus.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorVTail.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorWide.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/QuadRotorX.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Rover.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Vectored6DofUUV.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/VTOLDuoRotorTailSitter.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/VTOLPlane.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/VTOLPlaneOcto.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/VTOLQuadRotorTailSitter.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/VTOLTiltRotor.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Y6A.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/Y6B.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/YMinus.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/YPlus.svg"
)

qt_add_resources(${CMAKE_PROJECT_NAME} autopilot_plugin_common_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/AirframeComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/APMode.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/ArrowCCW.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/ArrowCW.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/ArrowDirection.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/CameraComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/FlightModesComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/MotorComponentIcon.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/PowerComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/RadioComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/SafetyComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/SensorsComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/StationMode.svg"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/TuningComponentIcon.png"
        "${CMAKE_SOURCE_DIR}/src/AutoPilotPlugins/Common/Images/wifi.svg"
)
