# ============================================================================
# ROVCompanion Module  (MIST MAVIROV)
# One-click SSH bring-up (MAVProxy + cameras on the Pi), Teensy servo arm
# control over UDP, and camera switching state for the Fly view panel.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ROVCompanionController.cc
        ROVCompanionController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
