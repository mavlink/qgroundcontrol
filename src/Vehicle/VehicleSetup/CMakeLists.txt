# ============================================================================
# Vehicle Setup Subsystem
# Vehicle configuration, calibration, and firmware upgrade
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JoystickConfigController.cc
        JoystickConfigController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Firmware Upgrade (requires serial link)
# ----------------------------------------------------------------------------
if(NOT QGC_NO_SERIAL_LINK)
    add_subdirectory(Firmware)

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            RCCalibrationStateMachine.cc
            RCCalibrationStateMachine.h
            RemoteControlCalibrationController.cc
            RemoteControlCalibrationController.h
    )
endif()

# ----------------------------------------------------------------------------
# Vehicle Setup QML Module
# ----------------------------------------------------------------------------
qt_add_library(VehicleSetupModule STATIC)

qt_add_qml_module(VehicleSetupModule
    URI QGroundControl.VehicleSetup
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        Firmware/FirmwareUpgrade.qml
        JoystickComponent.qml
        JoystickComponentButtons.qml
        JoystickComponentButtonMonitor.qml
        JoystickComponentSettings.qml
        OpticalFlowSensor.qml
        RemoteControlCalibration.qml
        RemoteControlChannelMonitor.qml
        RemoteControlChannelValueDisplay.qml
        SetupParameterEditor.qml
        SetupView.qml
        VehicleSummary.qml
    NO_PLUGIN
)
