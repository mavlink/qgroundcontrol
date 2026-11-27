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
        Receiver/KeyframeIntervalMeterTest.cc
        Receiver/KeyframeIntervalMeterTest.h
        Receiver/RecordingPlanTest.cc
        Receiver/RecordingPlanTest.h
        QtMultimedia/SharedVideoFrameTest.cc
        QtMultimedia/SharedVideoFrameTest.h
        Receiver/StreamSupervisorTest.cc
        Receiver/StreamSupervisorTest.h
        Receiver/StreamWatchdogTest.cc
        Receiver/StreamWatchdogTest.h
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
        VideoManagerTestBase.cc
        VideoManagerTestBase.h
        VideoManagerVehicleTest.cc
        VideoManagerVehicleTest.h
        Qml/VideoQmlTest.cc
        Qml/VideoQmlTest.h
        Recording/VideoRecordingServiceTest.cc
        Recording/VideoRecordingServiceTest.h
        Receiver/VideoScreenshotTest.cc
        Receiver/VideoScreenshotTest.h
        Qml/VideoStreamOutputRenderTest.cc
        Qml/VideoStreamOutputRenderTest.h
        Streams/VideoStreamResolverTest.cc
        Streams/VideoStreamResolverTest.h
        Receiver/VideoStreamStatsMeterTest.cc
        Receiver/VideoStreamStatsMeterTest.h
        Receiver/VideoStreamUriTest.cc
        Receiver/VideoStreamUriTest.h
        Telemetry/VideoTelemetryOverlayTest.cc
        Telemetry/VideoTelemetryOverlayTest.h
        VideoTestHelpers.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(KeyframeIntervalMeterTest LABELS Unit)
add_qgc_test(QtMultimediaVideoReceiverTest LABELS Integration)
add_qgc_test(QtVideoRecorderTest LABELS Integration)
add_qgc_test(RecordingPlanTest LABELS Unit)
add_qgc_test(SharedVideoFrameTest LABELS Unit)
add_qgc_test(StreamSupervisorTest LABELS Unit)
add_qgc_test(StreamWatchdogTest LABELS Unit)
add_qgc_test(SubtitleWriterTest LABELS Integration Vehicle)
add_qgc_test(UVCCameraControllerTest LABELS Unit)
add_qgc_test(UVCDevicesTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
add_qgc_test(VideoManagerTest LABELS Unit)
add_qgc_test(VideoManagerVehicleTest LABELS Integration Vehicle)
add_qgc_test(VideoQmlTest LABELS Integration)
add_qgc_test(VideoRecordingServiceTest LABELS Unit)
add_qgc_test(VideoScreenshotTest LABELS Unit)
# Reads back what Qt Quick draws, and its software backend draws no video. On Linux the offscreen platform has OpenGL
# through GLX (Mesa llvmpipe, see cmake/QGCTest.cmake), which Qt Quick takes only when asked for "rhi": the platform
# tells Qt not to try otherwise. Elsewhere the test skips.
set(_video_render_env_mod "")
if(LINUX)
    set(_video_render_env_mod ENV_MODIFICATION "QT_QUICK_BACKEND=set:rhi" "QSG_RHI_BACKEND=set:opengl")
endif()
add_qgc_test(VideoStreamOutputRenderTest LABELS Integration NoSanitizer ${_video_render_env_mod})
add_qgc_test(VideoStreamResolverTest LABELS Unit)
add_qgc_test(VideoStreamStatsMeterTest LABELS Unit)
add_qgc_test(VideoStreamUriTest LABELS Unit)
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

# The experimental FFmpeg receiver's streams are sent by GStreamer too.
get_property(_qgc_ffmpeg_video_built GLOBAL PROPERTY QGC_FFMPEG_VIDEO_BUILT)
if(_qgc_ffmpeg_video_built AND QGC_ENABLE_GST_VIDEOSTREAMING)
    target_sources(
        QGCUnitTests
        PRIVATE
            FFmpeg/FFmpegVideoReceiverStreamTest.cc
            FFmpeg/FFmpegVideoReceiverStreamTest.h
            FFmpeg/VideoReceiverDelayTest.cc
            FFmpeg/VideoReceiverDelayTest.h
    )
    target_link_libraries(
        QGCUnitTests
        PRIVATE
            FFmpeg::avformat
            FFmpeg::avcodec
            FFmpeg::avutil
    )
    # Its leak check keeps FFmpeg's allocations in: the receiver's, and the test's own, are among them.
    set(_ffmpeg_receiver_env_mod "")
    if(QGC_ENABLE_ASAN)
        set(_ffmpeg_receiver_env_mod
            ENV_MODIFICATION "LSAN_OPTIONS=set:suppressions=${CMAKE_BINARY_DIR}/asan_suppressions_own_ffmpeg.txt"
        )
    endif()
    add_qgc_test(FFmpegVideoReceiverStreamTest LABELS Integration ${_ffmpeg_receiver_env_mod})
    add_qgc_test(VideoReceiverDelayTest LABELS Integration Slow TIMEOUT 600)
endif()
