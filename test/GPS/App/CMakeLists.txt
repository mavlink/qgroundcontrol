# ============================================================================
# GPS Application Integration Tests
# Tests for the GPS sources compiled into the application
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSManagerTest.cc
            GPSMAVLinkOutputTest.cc
            GPSMAVLinkOutputTest.h
            GPSReceiverSettingsTest.cc
            GPSReceiverSettingsTest.h
            PositionManagerTest.cc
            PositionManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSManagerTest LABELS Unit GPS)
add_qgc_test(GPSMAVLinkOutputTest LABELS Integration GPS Vehicle)
add_qgc_test(GPSReceiverSettingsTest LABELS Unit GPS RTK)
add_qgc_test(PositionManagerTest LABELS Unit GPS)
