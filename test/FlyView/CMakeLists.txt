# ============================================================================
# FlyView Tests
# Tests for FlyView QML support objects
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ProximityRadarValuesTest.cc
        ProximityRadarValuesTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(ProximityRadarValuesTest LABELS Integration Vehicle)
