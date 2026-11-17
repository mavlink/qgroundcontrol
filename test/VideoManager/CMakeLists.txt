# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(
    QGCUnitTests
    PRIVATE
        QtMultimediaPrivateApiCheck.cc
        QtMultimedia/QtMultimediaVideoReceiverTest.cc
        QtMultimedia/QtMultimediaVideoReceiverTest.h
        QtMultimedia/QtVideoRecorderTest.cc
        QtMultimedia/QtVideoRecorderTest.h
        Receiver/RecordingPlanTest.cc
        Receiver/RecordingPlanTest.h
        QtMultimedia/SharedVideoFrameTest.cc
        QtMultimedia/SharedVideoFrameTest.h
        Receiver/StreamSupervisorTest.cc
        Receiver/StreamSupervisorTest.h
        Recording/SubtitleWriterTest.cc
        Recording/SubtitleWriterTest.h
        UVC/UVCCameraControllerTest.cc
        UVC/UVCCameraControllerTest.h
        UVC/UVCDevicesTest.cc
        UVC/UVCDevicesTest.h
        UVC/UVCStreamMonitorTest.cc
        UVC/UVCStreamMonitorTest.h
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerTest.cc
        VideoManagerTest.h
        VideoManagerVehicleTest.cc
        VideoManagerVehicleTest.h
        Recording/VideoRecordingServiceTest.cc
        Recording/VideoRecordingServiceTest.h
        Receiver/VideoScreenshotTest.cc
        Receiver/VideoScreenshotTest.h
        Streams/VideoStreamResolverTest.cc
        Streams/VideoStreamResolverTest.h
        Receiver/VideoStreamStatsMeterTest.cc
        Receiver/VideoStreamStatsMeterTest.h
        VideoTelemetryOverlayTest.cc
        VideoTelemetryOverlayTest.h
        VideoTestHelpers.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QtMultimediaVideoReceiverTest LABELS Integration TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(QtVideoRecorderTest LABELS Integration)
add_qgc_test(RecordingPlanTest LABELS Unit)
add_qgc_test(SharedVideoFrameTest LABELS Unit)
add_qgc_test(StreamSupervisorTest LABELS Unit)
add_qgc_test(SubtitleWriterTest LABELS Integration Vehicle)
add_qgc_test(UVCCameraControllerTest LABELS Unit)
add_qgc_test(UVCDevicesTest LABELS Unit)
add_qgc_test(UVCStreamMonitorTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoManagerVehicleTest LABELS Integration Vehicle)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoScreenshotTest LABELS Unit)
add_qgc_test(VideoStreamResolverTest LABELS Unit)
add_qgc_test(VideoStreamStatsMeterTest LABELS Unit)
add_qgc_test(VideoTelemetryOverlayTest LABELS Integration Vehicle)
