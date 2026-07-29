# ============================================================================
# Position Manager Unit Tests
# Tests for GCS position source management (NMEA source lifecycle)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PositionManagerTest.cc
        PositionManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(PositionManagerTest LABELS Unit)
