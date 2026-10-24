# QGC tests add sources to QGCUnitTests (test/Vehicle/, test/MissionManager/); keep these tests on the
# same target.

target_sources(
    QGCUnitTests
    PRIVATE
        GStreamerEnvironmentTest.cc
        GStreamerEnvironmentTest.h
        GStreamerInitTest.cc
        GStreamerInitTest.h
        GStreamerTest.cc
        GStreamerTest.h
        GStreamerTestBase.cc
        GStreamerTestBase.h
        GStreamerTestHelpers.cc
        GStreamerTestHelpers.h
        HwBuffers/GStreamerHwFrameTexturesTest.cc
        HwBuffers/GStreamerHwImportCacheTest.cc
        HwBuffers/GstHwBuffersPipelineTest.h
        HwBuffers/GstHwBuffersTest.h
        HwBuffers/common/GStreamerHwBuffersCommonTest.cc
        HwBuffers/d3d/GStreamerD3DTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufGlTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufGlTest.h
        HwBuffers/dmabuf/GStreamerDmaBufTest.cc
        HwBuffers/gl/GStreamerGlTest.cc
        HwBuffers/vulkan/GStreamerVulkanTest.cc
        Receiver/GstRecordingSinkTest.cc
        Receiver/GstRecordingSinkTest.h
        Receiver/GstVideoReceiverComponentsTest.cc
        Receiver/GstVideoReceiverComponentsTest.h
        Receiver/GstVideoReceiverTest.cc
        Receiver/GstVideoReceiverTest.h
        SourceFactory/GstSourceFactoryStreamTest.cc
        SourceFactory/GstSourceFactoryStreamTest.h
        SourceFactory/GstSourceFactoryTest.cc
        SourceFactory/GstSourceFactoryTest.h
        gstqgc/GstQgcSinkPipelineTest.cc
        gstqgc/GstQgcSinkPipelineTest.h
        gstqgc/GstQgcSinkTest.cc
        gstqgc/GstQgcSinkTest.h
        gstqgc/GstQgcTestSupport.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Pipeline-free cases run in the Unit pass; classes that play pipelines, open sockets or use the GPU are Integration, so
# a crash in a GPU path only takes down its own class.
add_qgc_test(GStreamerEnvironmentTest LABELS Unit)
add_qgc_test(GStreamerInitTest LABELS Unit)
add_qgc_test(GStreamerTest LABELS Unit)
add_qgc_test(GstHwBuffersTest LABELS Unit)
add_qgc_test(GstQgcSinkTest LABELS Unit)
add_qgc_test(GstSourceFactoryTest LABELS Unit)
add_qgc_test(GstVideoReceiverComponentsTest LABELS Unit)
add_qgc_test(GstHwBuffersPipelineTest LABELS Integration GPU)
add_qgc_test(GstQgcSinkPipelineTest LABELS Integration)
add_qgc_test(GstRecordingSinkTest LABELS Integration)
add_qgc_test(GstSourceFactoryStreamTest LABELS Integration)
add_qgc_test(GstVideoReceiverTest LABELS Integration)
# Needs Qt on EGL, which the offscreen QPA lacks: xcb with xcb_egl, falling back to offscreen (and skipping) without an
# X display.
set(_qgc_dmabuf_gl_env ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb" "QT_XCB_GL_INTEGRATION=set:xcb_egl")
add_qgc_test(GStreamerDmaBufGlTest LABELS Integration GPU ${_qgc_dmabuf_gl_env})

# MockVideoStreamServer is only compiled into single-config Debug builds (see its directory's CMakeLists.txt).
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(
        SOURCE Receiver/GstVideoReceiverTest.cc TARGET_DIRECTORY QGCUnitTests
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    )
endif()
