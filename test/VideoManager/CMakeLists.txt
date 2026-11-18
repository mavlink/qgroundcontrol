# ============================================================================
# VideoManager Tests
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        QtMultimediaPrivateApiCheck.cc
        QtMultimedia/QtMultimediaReceiverTestSupport.h
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

add_qgc_test(QtMultimediaVideoReceiverTest LABELS Integration)
add_qgc_test(QtVideoRecorderTest LABELS Integration)
add_qgc_test(RecordingPlanTest LABELS Unit)
add_qgc_test(SharedVideoFrameTest LABELS Unit)
add_qgc_test(StreamSupervisorTest LABELS Unit)
add_qgc_test(SubtitleWriterTest LABELS Integration Vehicle)
add_qgc_test(UVCCameraControllerTest LABELS Unit)
add_qgc_test(UVCDevicesTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoManagerVehicleTest LABELS Integration Vehicle)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoScreenshotTest LABELS Unit)
add_qgc_test(VideoStreamResolverTest LABELS Unit)
add_qgc_test(VideoStreamStatsMeterTest LABELS Unit)
add_qgc_test(VideoTelemetryOverlayTest LABELS Integration Vehicle)

# Without GStreamer its tests aren't built, nor those of a receiver playing a stream: GStreamer sends the stream,
# whichever backend the receiver is.
if(QGC_ENABLE_GST_VIDEOSTREAMING)
    add_subdirectory(GStreamer)
    target_sources(
        QGCUnitTests
        PRIVATE
            QtMultimedia/QtMultimediaVideoReceiverStreamTest.cc
            QtMultimedia/QtMultimediaVideoReceiverStreamTest.h
            Receiver/VideoReceiverContractTest.cc
            Receiver/VideoReceiverContractTest.h
    )
    add_qgc_test(QtMultimediaVideoReceiverStreamTest LABELS Integration)
    add_qgc_test(VideoReceiverContractTest LABELS Integration)
endif()
