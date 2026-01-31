# ============================================================================
# Firmware Upgrade Unit Tests
# Tests for firmware upgrade state machine and controller
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FirmwareUpgradeStateMachineTest.cc
        FirmwareUpgradeStateMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
