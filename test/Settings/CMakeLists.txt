# ============================================================================
# Settings Unit Tests
# Tests for application settings groups and their validation logic
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        RemoteIDSettingsTest.cc
        RemoteIDSettingsTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RemoteIDSettingsTest LABELS Unit)
