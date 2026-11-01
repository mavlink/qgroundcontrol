# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(
    QGCUnitTests
    PRIVATE
        QtMultimediaPrivateApiCheck.cc
        QtMultimediaVideoReceiverTest.cc
        QtMultimediaVideoReceiverTest.h
        QtVideoRecorderTest.cc
        QtVideoRecorderTest.h
        SubtitleWriterTest.cc
        SubtitleWriterTest.h
        UVCCameraControllerTest.cc
        UVCCameraControllerTest.h
        UVCDevicesTest.cc
        UVCDevicesTest.h
        UVCFrameRelayTest.cc
        UVCFrameRelayTest.h
        UVCStreamMonitorTest.cc
        UVCStreamMonitorTest.h
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerTest.cc
        VideoManagerTest.h
        VideoRecordingServiceTest.cc
        VideoRecordingServiceTest.h
        VideoScreenshotTest.cc
        VideoScreenshotTest.h
        VideoStreamResolverTest.cc
        VideoStreamResolverTest.h
        VideoStreamStatsMeterTest.cc
        VideoStreamStatsMeterTest.h
        VideoTelemetryOverlayTest.cc
        VideoTelemetryOverlayTest.h
        VideoTestHelpers.h
)

add_qgc_test(QtMultimediaVideoReceiverTest LABELS Integration)
add_qgc_test(QtVideoRecorderTest LABELS Integration)
add_qgc_test(SubtitleWriterTest LABELS Integration Vehicle)
add_qgc_test(UVCCameraControllerTest LABELS Unit)
add_qgc_test(UVCDevicesTest LABELS Unit)
add_qgc_test(UVCFrameRelayTest LABELS Unit)
add_qgc_test(UVCStreamMonitorTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoScreenshotTest LABELS Unit)
add_qgc_test(VideoStreamResolverTest LABELS Unit)
add_qgc_test(VideoStreamStatsMeterTest LABELS Unit)
add_qgc_test(VideoTelemetryOverlayTest LABELS Integration Vehicle)

# MockVideoStreamServer is only compiled into single-config Debug builds (see its directory's CMakeLists.txt).
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(
        SOURCE QtMultimediaVideoReceiverTest.cc TARGET_DIRECTORY QGCUnitTests
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    )
endif()
