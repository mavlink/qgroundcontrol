# ============================================================================
# Settings Unit Tests
# Tests for application settings groups and their validation logic
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AppSettingsTest.cc
        AppSettingsTest.h
        AutoConnectSettingsTest.cc
        AutoConnectSettingsTest.h
        GeoViewSettingsTest.cc
        GeoViewSettingsTest.h
        RemoteIDSettingsTest.cc
        RemoteIDSettingsTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(AppSettingsTest LABELS Unit)
add_qgc_test(AutoConnectSettingsTest LABELS Unit)
add_qgc_test(GeoViewSettingsTest LABELS Unit)
add_qgc_test(RemoteIDSettingsTest LABELS Unit)
