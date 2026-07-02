# ============================================================================
# AutoPilot Plugins Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4/AirframeComponentAirframesTest.cc
        PX4/AirframeComponentAirframesTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PX4)

add_qgc_test(AirframeComponentAirframesTest LABELS Unit)
