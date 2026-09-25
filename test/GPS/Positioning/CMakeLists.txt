# ============================================================================
# Positioning Unit Tests
# Tests for GPS observations, source health, and GCS position source selection
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSPositionServiceTest.cc GPSPositionServiceTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSPositionServiceTest LABELS Unit GPS)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealthTest.cc GPSSourceHealthTest.h)
add_qgc_test(GPSSourceHealthTest LABELS Unit GPS)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSAcceptedStateTest.cc)
add_qgc_test(GPSAcceptedStateTest LABELS Unit GPS)
