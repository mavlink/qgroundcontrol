# ============================================================================
# ADS-B Module
# Automatic Dependent Surveillance-Broadcast for air traffic awareness
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ADSBTCPLink.cc
        ADSBTCPLink.h
        ADSBVehicle.cc
        ADSBVehicle.h
        ADSBVehicleManager.cc
        ADSBVehicleManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
