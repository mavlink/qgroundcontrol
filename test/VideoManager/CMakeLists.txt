# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(
    QGCUnitTests
    PRIVATE
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerTest.cc
        VideoManagerTest.h
        VideoRecordingServiceTest.cc
        VideoRecordingServiceTest.h
        VideoStreamResolverTest.cc
        VideoStreamResolverTest.h
        VideoTestHelpers.h
)

add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoStreamResolverTest LABELS Unit)
