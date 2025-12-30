# ============================================================================
# ADSB Unit Tests
# Tests for ADS-B traffic detection and collision avoidance
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ADSBTest.cc
        ADSBTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# TODO: Enable ADSB simulator test resources
# qt_add_resources(${CMAKE_PROJECT_NAME} "ADSBTest_res"
#     PREFIX "/unittest"
#     FILES
#         ADSB_Simulator.py
# )
