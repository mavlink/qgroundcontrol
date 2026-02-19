# ============================================================================
# GStreamer Integration Tests
# Requires GStreamer runtime; tests skip gracefully when GST is disabled.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamerTest.cc
        GStreamerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
