# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(
    QGCUnitTests
    PRIVATE
        QtMultimediaPrivateApiCheck.cc
        SubtitleWriterTest.cc
        SubtitleWriterTest.h
        UVCCameraControllerTest.cc
        UVCCameraControllerTest.h
        UVCDevicesTest.cc
        UVCDevicesTest.h
        UVCStreamMonitorTest.cc
        UVCStreamMonitorTest.h
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerTest.cc
        VideoManagerTest.h
        VideoRecordingServiceTest.cc
        VideoRecordingServiceTest.h
        VideoStreamResolverTest.cc
        VideoStreamResolverTest.h
        VideoStreamStatsMeterTest.cc
        VideoStreamStatsMeterTest.h
        VideoTelemetryOverlayTest.cc
        VideoTelemetryOverlayTest.h
        VideoTestHelpers.h
)

add_qgc_test(SubtitleWriterTest LABELS Integration Vehicle)
add_qgc_test(UVCCameraControllerTest LABELS Unit)
add_qgc_test(UVCDevicesTest LABELS Unit)
add_qgc_test(UVCStreamMonitorTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoStreamResolverTest LABELS Unit)
add_qgc_test(VideoStreamStatsMeterTest LABELS Unit)
add_qgc_test(VideoTelemetryOverlayTest LABELS Integration Vehicle)
