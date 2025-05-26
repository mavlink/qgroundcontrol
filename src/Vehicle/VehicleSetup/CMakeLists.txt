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

# qt_add_qml_module(QGC
#     URI QGroundControl.VehicleSetup
#     VERSION 1.0
#     QML_FILES
#       FirmwareUpgrade.qml
#       JoystickConfig.qml
#       JoystickConfigAdvanced.qml
#       JoystickConfigButtons.qml
#       JoystickConfigCalibration.qml
#       JoystickConfigGeneral.qml
#       OpticalFlowSensor.qml
#       SetupParameterEditor.qml
#       SetupView.qml
#       VehicleSummary.qml
#   RESOURCES
#       FirmwareUpgradeIcon.png
#       VehicleSummaryIcon.png
#     OUTPUT_TARGETS QGCLocation_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtQuick.Dialogs
#       QtQuick.Layouts

#       QGroundControl
#       QGroundControl.Controls
#       QGroundControl.FactSystem
#       QGroundControl.FactControls
#       QGroundControl.Palette
#       QGroundControl.Controllers
#       QGroundControl.ScreenTools
#       QGroundControl.MultiVehicleManager
#       QGroundControl.AutoPilotPlugin
# )
