# ============================================================================
# Firmware Upgrade Subsystem
# Bootloader communication, firmware download and flashing
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Bootloader.cc
        Bootloader.h
        FirmwareImage.cc
        FirmwareImage.h
        FirmwareUpgradeController.cc
        FirmwareUpgradeController.h
        FirmwareUpgradeStateMachine.cc
        FirmwareUpgradeStateMachine.h
        PX4FirmwareUpgradeThread.cc
        PX4FirmwareUpgradeThread.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
