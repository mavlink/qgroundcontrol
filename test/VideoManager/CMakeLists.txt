# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
)

add_qgc_test(VideoManagerInitTest LABELS Unit)
