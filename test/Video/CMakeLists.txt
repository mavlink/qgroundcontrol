# ============================================================================
# Video Unit Tests
# Tests for VideoReceiver, VideoSettings, and URL construction
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoReceiverTest.cc
        VideoReceiverTest.h
        VideoSettingsTest.cc
        VideoSettingsTest.h
        VideoUrlConstructionTest.cc
        VideoUrlConstructionTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
