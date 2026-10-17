# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(
    QGCUnitTests
    PRIVATE
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerRestartTest.cc
        VideoManagerRestartTest.h
)

add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerRestartTest LABELS Unit)
