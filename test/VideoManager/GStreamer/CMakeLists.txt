# QGC tests add sources to QGCUnitTests (test/Vehicle/, test/MissionManager/); keep these tests on the
# same target.

target_sources(
    QGCUnitTests
    PRIVATE
        GStreamerEnvironmentTest.cc
        GStreamerEnvironmentTest.h
        GStreamerHelpersTest.cc
        GStreamerHelpersTest.h
        GStreamerInitTest.cc
        GStreamerInitTest.h
        GStreamerTest.cc
        GStreamerTest.h
        GStreamerTestBase.cc
        GStreamerTestBase.h
        GStreamerTestHelpers.cc
        GStreamerTestHelpers.h
        GstSelfTestPipelineTest.cc
        GstSelfTestPipelineTest.h
        HwBuffers/GStreamerHwFrameTexturesTest.cc
        HwBuffers/GStreamerHwImportCacheTest.cc
        HwBuffers/GstHwBuffersGlReadbackTest.cc
        HwBuffers/GstHwBuffersGlReadbackTest.h
        HwBuffers/GstHwBuffersPipelineTest.cc
        HwBuffers/GstHwBuffersPipelineTest.h
        HwBuffers/GstHwBuffersTest.cc
        HwBuffers/GstHwBuffersTest.h
        HwBuffers/GstHwImageReadback.h
        HwBuffers/apple/GStreamerIOSurfaceTest.cc
        HwBuffers/d3d/GStreamerD3DTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufTest.cc
        HwBuffers/gl/GStreamerGlTest.cc
        HwBuffers/vulkan/GStreamerVulkanTest.cc
        Receiver/GstRecordingSinkTest.cc
        Receiver/GstRecordingSinkTest.h
        Receiver/GstVideoReceiverComponentsTest.cc
        Receiver/GstVideoReceiverComponentsTest.h
        Receiver/GstVideoReceiverPipelineTest.cc
        Receiver/GstVideoReceiverPipelineTest.h
        Receiver/GstVideoReceiverReconnectTest.cc
        Receiver/GstVideoReceiverReconnectTest.h
        Receiver/GstVideoReceiverRecordingTest.cc
        Receiver/GstVideoReceiverRecordingTest.h
        Receiver/GstVideoReceiverStatsTest.cc
        Receiver/GstVideoReceiverStatsTest.h
        Receiver/GstVideoReceiverTestBase.cc
        Receiver/GstVideoReceiverTestBase.h
        SourceFactory/GstSourceFactoryStreamTest.cc
        SourceFactory/GstSourceFactoryStreamTest.h
        SourceFactory/GstSourceFactoryTest.cc
        SourceFactory/GstSourceFactoryTest.h
        gstqgc/GstQgcSinkControllerTest.cc
        gstqgc/GstQgcSinkControllerTest.h
        gstqgc/GstQgcSinkPipelineTest.cc
        gstqgc/GstQgcSinkPipelineTest.h
        gstqgc/GstQgcSinkTest.cc
        gstqgc/GstQgcSinkTest.h
        gstqgc/GstQgcTestSupport.cc
        gstqgc/GstQgcTestSupport.h
        gstqgc/GstQgcVideoFormatsTest.cc
        gstqgc/GstQgcVideoFormatsTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Encoded streams the tests send instead of encoding their own (GstTest::FixtureRtpSender): the app's self-test
# samples, made by src/VideoManager/GStreamer/Runtime/Media/generate.sh.
if(QGC_ENABLE_GST_VIDEOSTREAMING)
    set(_qgc_gst_test_media "${CMAKE_SOURCE_DIR}/src/VideoManager/GStreamer/Runtime/Media")
    qt_add_resources(
        ${CMAKE_PROJECT_NAME} "GStreamerTestMedia_res"
        PREFIX "/unittest/video"
        BASE "${_qgc_gst_test_media}"
        FILES
            "${_qgc_gst_test_media}/h264_160x120.h264"
            "${_qgc_gst_test_media}/h264_320x240.h264"
            "${_qgc_gst_test_media}/h265_320x240.h265"
    )
endif()

# Pipeline-free cases run in the Unit pass; classes that play pipelines, open sockets or use the GPU are Integration, so
# a crash in a GPU path only takes down its own class.
add_qgc_test(GStreamerEnvironmentTest LABELS Unit)
add_qgc_test(GStreamerInitTest LABELS Unit)
add_qgc_test(GStreamerTest LABELS Unit)
add_qgc_test(GstHwBuffersTest LABELS Unit)
add_qgc_test(GstQgcSinkControllerTest LABELS Unit)
add_qgc_test(GstQgcSinkTest LABELS Unit)
add_qgc_test(GstQgcVideoFormatsTest LABELS Unit)
add_qgc_test(GstSourceFactoryTest LABELS Unit)
add_qgc_test(GstVideoReceiverComponentsTest LABELS Unit)
add_qgc_test(GStreamerHelpersTest LABELS Integration)
add_qgc_test(GstHwBuffersPipelineTest LABELS Integration GPU)
add_qgc_test(GstQgcSinkPipelineTest LABELS Integration)
add_qgc_test(GstRecordingSinkTest LABELS Integration)
add_qgc_test(GstSelfTestPipelineTest LABELS Integration)
add_qgc_test(GstSourceFactoryStreamTest LABELS Integration)
add_qgc_test(GstVideoReceiverPipelineTest LABELS Integration)
add_qgc_test(GstVideoReceiverReconnectTest LABELS Integration)
add_qgc_test(GstVideoReceiverRecordingTest LABELS Integration)
add_qgc_test(GstVideoReceiverStatsTest LABELS Integration)
# Needs Qt on EGL, which the offscreen QPA lacks: xcb with xcb_egl, falling back to offscreen (and skipping) without an
# X display.
set(_qgc_gl_readback_env ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb" "QT_XCB_GL_INTEGRATION=set:xcb_egl")
add_qgc_test(GstHwBuffersGlReadbackTest LABELS Integration GPU ${_qgc_gl_readback_env})

# The GstHarness tests need gst-check, which the GStreamer layer finds only for test builds and only where the SDK or
# distribution ships it (src/VideoManager/GStreamer/CMakeLists.txt).
if(TARGET GStreamer::Check)
    target_sources(
        QGCUnitTests
        PRIVATE
            SourceFactory/GstSourceFactoryHarnessTest.cc
            SourceFactory/GstSourceFactoryHarnessTest.h
            gstqgc/GstQgcSinkHarnessTest.cc
            gstqgc/GstQgcSinkHarnessTest.h
    )
    add_qgc_test(GstQgcSinkHarnessTest LABELS Unit)
    add_qgc_test(GstSourceFactoryHarnessTest LABELS Unit)
endif()

# GStreamerTest checks the runtime GStreamer against the floor the build accepts: the system minimum on Linux, the
# official SDKs' elsewhere.
if(LINUX)
    qgc_config_read(_qgc_gst_runtime_minimum gstreamer version minimum)
else()
    qgc_config_read(_qgc_gst_runtime_minimum gstreamer version bundled_minimum)
endif()
set_property(
    SOURCE GStreamerTest.cc TARGET_DIRECTORY QGCUnitTests
    APPEND
    PROPERTY COMPILE_DEFINITIONS "QGC_GST_RUNTIME_MINIMUM_VERSION=\"${_qgc_gst_runtime_minimum}\""
)

# MockVideoStreamServer is only compiled into single-config Debug builds (see its directory's CMakeLists.txt).
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(
        SOURCE Receiver/GstVideoReceiverPipelineTest.cc TARGET_DIRECTORY QGCUnitTests
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    )
endif()
