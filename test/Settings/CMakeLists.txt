# ============================================================================
# Settings Unit Tests
# Tests for application settings groups and their validation logic
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE AppSettingsTest.cc
            AppSettingsTest.h
            FlyViewSettingsTest.cc
            FlyViewSettingsTest.h
            GPSCorrectionSettingsTest.cc
            GPSCorrectionSettingsTest.h
            RemoteIDSettingsTest.cc
            RemoteIDSettingsTest.h
            RTKSettingsTest.cc
            RTKSettingsTest.h
            SettingsManagerTest.cc
            SettingsManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(AppSettingsTest LABELS Unit)
add_qgc_test(FlyViewSettingsTest LABELS Unit)
add_qgc_test(GPSCorrectionSettingsTest LABELS Unit GPS)
add_qgc_test(RemoteIDSettingsTest LABELS Unit)
add_qgc_test(RTKSettingsTest LABELS Unit GPS)
add_qgc_test(SettingsManagerTest LABELS Unit)
