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
        HwBuffers/GstHwBuffersTest.cc
        HwBuffers/GstHwBuffersTest.h
        HwBuffers/GstHwImageReadback.h
        HwBuffers/gl/GstHwBuffersGlTest.cc
        HwBuffers/gl/GstHwBuffersGlTest.h
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
        Source/RtpPacketCounterTest.cc
        Source/RtpPacketCounterTest.h
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

# Sink, so that "gstqgc/GstQgcTestSupport.h" resolves as the sink's headers do in the sources
# ("gstqgc/GstQgcSinkPolicy.h").
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
add_qgc_test(RtpPacketCounterTest LABELS Unit)
add_qgc_test(GStreamerHelpersTest LABELS Integration)
add_qgc_test(GstHwBuffersGlTest LABELS Integration GPU)
add_qgc_test(GstQgcSinkPipelineTest LABELS Integration)
add_qgc_test(GstRecordingSinkTest LABELS Integration)
add_qgc_test(GstSelfTestPipelineTest LABELS Integration)
add_qgc_test(GstSourceFactoryStreamTest LABELS Integration)
add_qgc_test(GstVideoReceiverPipelineTest LABELS Integration)
add_qgc_test(GstVideoReceiverReconnectTest LABELS Integration)
add_qgc_test(GstVideoReceiverRecordingTest LABELS Integration SHARDS 2)
add_qgc_test(GstVideoReceiverStatsTest LABELS Integration)

# A GPU path's tests are built only where the GStreamer layer compiled the path
# (src/VideoManager/GStreamer/HwBuffers/CMakeLists.txt): a test that skips then tells of the host, not of the build.
function(_qgc_gst_test_has result definition)
    get_target_property(_definitions QGCGStreamer INTERFACE_COMPILE_DEFINITIONS)
    if("${definition}=1" IN_LIST _definitions)
        set(${result}
            TRUE
            PARENT_SCOPE
        )
    else()
        set(${result}
            FALSE
            PARENT_SCOPE
        )
    endif()
endfunction()
_qgc_gst_test_has(_qgc_test_any_gpu_path QGC_HAS_ANY_GPU_PATH)
_qgc_gst_test_has(_qgc_test_dmabuf QGC_HAS_GST_DMABUF_GPU_PATH)
_qgc_gst_test_has(_qgc_test_dmabuf_vulkan QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
_qgc_gst_test_has(_qgc_test_glmemory QGC_HAS_GST_GLMEMORY_GPU_PATH)
_qgc_gst_test_has(_qgc_test_iosurface QGC_HAS_GST_IOSURFACE_GPU_PATH)
_qgc_gst_test_has(_qgc_test_d3d11 QGC_HAS_GST_D3D11_GPU_PATH)
_qgc_gst_test_has(_qgc_test_d3d12 QGC_HAS_GST_D3D12_GPU_PATH)

# moc reads these definitions in the test headers, and the classes built follow them, but nothing runs moc again when
# they change in a build directory that exists: its command depends on the files it read alone. Without its timestamp it
# runs.
get_target_property(_qgc_gst_definitions QGCGStreamer INTERFACE_COMPILE_DEFINITIONS)
set(_qgc_gst_definitions_file "${CMAKE_CURRENT_BINARY_DIR}/gst-definitions.txt")
set(_qgc_gst_previous_definitions "")
if(EXISTS "${_qgc_gst_definitions_file}")
    file(READ "${_qgc_gst_definitions_file}" _qgc_gst_previous_definitions)
endif()
if(NOT _qgc_gst_previous_definitions STREQUAL "${_qgc_gst_definitions}")
    file(WRITE "${_qgc_gst_definitions_file}" "${_qgc_gst_definitions}")
    get_target_property(_qgc_unit_tests_dir QGCUnitTests BINARY_DIR)
    file(REMOVE "${_qgc_unit_tests_dir}/QGCUnitTests_autogen/timestamp")
endif()

if(_qgc_test_any_gpu_path)
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/GstHwFrameTexturesTest.cc
            HwBuffers/GstHwFrameTexturesTest.h
    )
    add_qgc_test(GstHwFrameTexturesTest LABELS Unit)
endif()

if(_qgc_test_dmabuf)
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/dmabuf/GstHwBuffersDmaBufTest.cc
            HwBuffers/dmabuf/GstHwBuffersDmaBufTest.h
    )
    add_qgc_test(GstHwBuffersDmaBufTest LABELS Unit)
    add_qgc_test(GstHwBuffersDmaBufPipelineTest LABELS Integration GPU)
endif()

if(WIN32 AND (_qgc_test_d3d11 OR _qgc_test_d3d12))
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/d3d/GstHwBuffersD3DTest.cc
            HwBuffers/d3d/GstHwBuffersD3DTest.h
    )
    add_qgc_test(GstHwBuffersD3DTest LABELS Integration GPU)
endif()

# The path is iOS's too; the test decodes with VideoToolbox's desktop decoder.
if(MACOS AND _qgc_test_iosurface)
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/apple/GstHwBuffersIOSurfaceTest.cc
            HwBuffers/apple/GstHwBuffersIOSurfaceTest.h
    )
    add_qgc_test(GstHwBuffersIOSurfaceTest LABELS Integration GPU)
endif()

if(LINUX AND (_qgc_test_dmabuf OR _qgc_test_glmemory))
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/GstHwBuffersGlReadbackTest.cc
            HwBuffers/GstHwBuffersGlReadbackTest.h
    )
    # Needs Qt on EGL, which the offscreen QPA lacks: xcb with xcb_egl, falling back to offscreen (and skipping)
    # without an X display.
    add_qgc_test(
        GstHwBuffersGlReadbackTest LABELS Integration GPU ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb"
                                                                           "QT_XCB_GL_INTEGRATION=set:xcb_egl"
    )
endif()

if(_qgc_test_dmabuf_vulkan)
    target_sources(
        QGCUnitTests
        PRIVATE
            HwBuffers/GstHwBuffersVulkanReadbackTest.cc
            HwBuffers/GstHwBuffersVulkanReadbackTest.h
    )
    # Needs a platform plugin with Vulkan support, which the offscreen QPA lacks too.
    add_qgc_test(
        GstHwBuffersVulkanReadbackTest LABELS Integration GPU ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb"
    )
endif()

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
