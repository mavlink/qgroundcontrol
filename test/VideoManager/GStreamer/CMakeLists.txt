# QGC tests add sources to QGCUnitTests (test/Vehicle/, test/MissionManager/); keep these tests on the
# same target.

target_sources(
    QGCUnitTests
    PRIVATE
        Runtime/GStreamerEnvironmentTest.cc
        Runtime/GStreamerEnvironmentTest.h
        Runtime/GStreamerHelpersTest.cc
        Runtime/GStreamerHelpersTest.h
        Runtime/GStreamerInitTest.cc
        Runtime/GStreamerInitTest.h
        Runtime/GStreamerTest.cc
        Runtime/GStreamerTest.h
        GStreamerTestBase.cc
        GStreamerTestBase.h
        GStreamerTestHelpers.cc
        GStreamerTestHelpers.h
        SelfTest/GstSelfTestPipelineTest.cc
        SelfTest/GstSelfTestPipelineTest.h
        HwBuffers/GStreamerHwFrameTexturesTest.cc
        HwBuffers/GStreamerHwImportCacheTest.cc
        HwBuffers/GstHwBuffersGlReadbackTest.cc
        HwBuffers/GstHwBuffersGlReadbackTest.h
        HwBuffers/GstHwBuffersPipelineTest.cc
        HwBuffers/GstHwBuffersPipelineTest.h
        HwBuffers/GstHwBuffersTest.cc
        HwBuffers/GstHwBuffersTest.h
        HwBuffers/GstHwBuffersVulkanReadbackTest.cc
        HwBuffers/GstHwBuffersVulkanReadbackTest.h
        HwBuffers/GstHwImageReadback.h
        HwBuffers/apple/GStreamerIOSurfaceTest.cc
        HwBuffers/d3d/GStreamerD3DTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufTest.cc
        HwBuffers/gl/GStreamerGlTest.cc
        Pipeline/GstRecordingSinkTest.cc
        Pipeline/GstRecordingSinkTest.h
        Pipeline/GstVideoReceiverComponentsTest.cc
        Pipeline/GstVideoReceiverComponentsTest.h
        Pipeline/GstVideoReceiverPipelineTest.cc
        Pipeline/GstVideoReceiverPipelineTest.h
        Pipeline/GstVideoReceiverReconnectTest.cc
        Pipeline/GstVideoReceiverReconnectTest.h
        Pipeline/GstVideoReceiverRecordingTest.cc
        Pipeline/GstVideoReceiverRecordingTest.h
        Pipeline/GstVideoReceiverStatsTest.cc
        Pipeline/GstVideoReceiverStatsTest.h
        Pipeline/GstVideoReceiverTestBase.cc
        Pipeline/GstVideoReceiverTestBase.h
        Source/GstSourceFactoryStreamTest.cc
        Source/GstSourceFactoryStreamTest.h
        Source/GstSourceFactoryTest.cc
        Source/GstSourceFactoryTest.h
        Sink/GstQgcSinkControllerTest.cc
        Sink/GstQgcSinkControllerTest.h
        Sink/gstqgc/GstQgcSinkPipelineTest.cc
        Sink/gstqgc/GstQgcSinkPipelineTest.h
        Sink/gstqgc/GstQgcSinkTest.cc
        Sink/gstqgc/GstQgcSinkTest.h
        Sink/gstqgc/GstQgcTestSupport.cc
        Sink/gstqgc/GstQgcTestSupport.h
        Sink/gstqgc/GstQgcVideoFormatsTest.cc
        Sink/gstqgc/GstQgcVideoFormatsTest.h
)

# Sink, so that "gstqgc/GstQgcTestSupport.h" resolves as the sink's headers do in the sources ("gstqgc/GstQgcCaps.h").
target_include_directories(
    QGCUnitTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Sink
)

# The encoded stream the tests send instead of encoding their own (GstTest::FixtureRtpSender), made by
# src/VideoManager/GStreamer/SelfTest/Media/generate.sh. The other two streams of that directory are the app's
# self-test samples, which the tests read from the app's resources.
set(_qgc_gst_test_media "${CMAKE_SOURCE_DIR}/src/VideoManager/GStreamer/SelfTest/Media")
qt_add_resources(
    ${CMAKE_PROJECT_NAME} "GStreamerTestMedia_res"
    PREFIX "/unittest/video"
    BASE "${_qgc_gst_test_media}"
    FILES
        "${_qgc_gst_test_media}/h264_320x240.h264"
)

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
add_qgc_test(GstVideoReceiverRecordingTest LABELS Integration SHARDS 2)
add_qgc_test(GstVideoReceiverStatsTest LABELS Integration)
# Needs Qt on EGL, which the offscreen QPA lacks: xcb with xcb_egl, falling back to offscreen (and skipping) without an
# X display.
set(_qgc_gl_readback_env ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb" "QT_XCB_GL_INTEGRATION=set:xcb_egl")
add_qgc_test(GstHwBuffersGlReadbackTest LABELS Integration GPU ${_qgc_gl_readback_env})
# Needs a platform plugin with Vulkan support, which the offscreen QPA lacks too.
add_qgc_test(
    GstHwBuffersVulkanReadbackTest LABELS Integration GPU ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb"
)

# The GstHarness tests need gst-check, which the GStreamer layer finds only for test builds and only where the SDK or
# distribution ships it (src/VideoManager/GStreamer/CMakeLists.txt).
if(TARGET GStreamer::Check)
    target_sources(
        QGCUnitTests
        PRIVATE
            Source/GstSourceFactoryHarnessTest.cc
            Source/GstSourceFactoryHarnessTest.h
            Sink/gstqgc/GstQgcSinkHarnessTest.cc
            Sink/gstqgc/GstQgcSinkHarnessTest.h
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
    SOURCE Runtime/GStreamerTest.cc TARGET_DIRECTORY QGCUnitTests
    APPEND
    PROPERTY COMPILE_DEFINITIONS "QGC_GST_RUNTIME_MINIMUM_VERSION=\"${_qgc_gst_runtime_minimum}\""
)
