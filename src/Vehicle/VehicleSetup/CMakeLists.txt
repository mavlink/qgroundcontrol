target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JoystickConfigController.cc
        JoystickConfigController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            Bootloader.cc
            Bootloader.h
            FirmwareImage.cc
            FirmwareImage.h
            FirmwareUpgradeController.cc
            FirmwareUpgradeController.h
            PX4FirmwareUpgradeThread.cc
            PX4FirmwareUpgradeThread.h
    )
endif()

qt_add_library(VehicleSetupModule STATIC)

qt_add_qml_module(VehicleSetupModule
    URI QGroundControl.VehicleSetup
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        FirmwareUpgrade.qml
        JoystickConfig.qml
        JoystickConfigAdvanced.qml
        JoystickConfigButtons.qml
        JoystickConfigCalibration.qml
        JoystickConfigGeneral.qml
        OpticalFlowSensor.qml
        SetupParameterEditor.qml
        SetupView.qml
        VehicleSummary.qml
    NO_PLUGIN
)
