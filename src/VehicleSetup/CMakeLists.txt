find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_library(VehicleSetup STATIC
    JoystickConfigController.cc
    JoystickConfigController.h
    VehicleComponent.cc
    VehicleComponent.h
)

target_link_libraries(VehicleSetup
    PRIVATE
        Qt6::Qml
        Compression
        FactSystem
        QGC
        Settings
        Utilities
        Vehicle
        VehicleComponents
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        FactControls
        Joystick
)

target_include_directories(VehicleSetup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(VehicleSetup
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

    target_link_libraries(VehicleSetup
        PRIVATE
            API
        PUBLIC
            Comms
        )
endif()

# qt_add_qml_module(VehicleSetup
#     URI QGroundControl.VehicleSetup
#     VERSION 1.0
#     QML_FILES
#       FirmwareUpgrade.qml
#       JoystickConfig.qml
#       JoystickConfigAdvanced.qml
#       JoystickConfigButtons.qml
#       JoystickConfigCalibration.qml
#       JoystickConfigGeneral.qml
#       PX4FlowSensor.qml
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
