# ============================================================================
# ADSB Unit Tests
# Tests for ADS-B traffic detection and collision avoidance
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ADSBVehicleTest.cc
        ADSBVehicleTest.h
        ADSBTCPLinkTest.cc
        ADSBTCPLinkTest.h
        ADSBVehicleManagerTest.cc
        ADSBVehicleManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
