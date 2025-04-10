set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/JoystickConfigController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/JoystickConfigController.h
    CACHE INTERNAL "" FORCE
)

if(NOT QGC_NO_SERIAL_LINK)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/Bootloader.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Bootloader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareImage.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareImage.h
        ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareUpgradeController.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareUpgradeController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwareUpgradeThread.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4FirmwareUpgradeThread.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

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
