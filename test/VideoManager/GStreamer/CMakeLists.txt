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
        GStreamerTestHelpers.cc
        GStreamerTestHelpers.h
        HwBuffers/GStreamerHwFrameTexturesTest.cc
        HwBuffers/GStreamerHwImportCacheTest.cc
        HwBuffers/common/GStreamerHwBuffersCommonTest.cc
        HwBuffers/d3d/GStreamerD3DTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufGlTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufGlTest.h
        HwBuffers/dmabuf/GStreamerDmaBufTest.cc
        HwBuffers/gl/GStreamerGlTest.cc
        HwBuffers/vulkan/GStreamerVulkanTest.cc
        Receiver/GstVideoReceiverComponentsTest.cc
        Receiver/GstVideoReceiverComponentsTest.h
        Receiver/GstVideoReceiverTest.cc
        Receiver/GstVideoReceiverTest.h
        SourceFactory/GStreamerSourceFactoryTest.cc
        gstqgc/GStreamerGstQgcTest.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GStreamerEnvironmentTest LABELS Unit)
add_qgc_test(GStreamerInitTest LABELS Unit)
add_qgc_test(GStreamerTest LABELS Integration)
add_qgc_test(GstVideoReceiverTest LABELS Integration)
add_qgc_test(GstVideoReceiverComponentsTest LABELS Unit)
# Needs Qt on EGL, which the offscreen QPA lacks: xcb with xcb_egl, falling back to offscreen (and skipping) without an
# X display.
set(_qgc_dmabuf_gl_env ENV_MODIFICATION "QT_QPA_PLATFORM=cmake_list_prepend:xcb" "QT_XCB_GL_INTEGRATION=set:xcb_egl")
add_qgc_test(GStreamerDmaBufGlTest LABELS Integration ${_qgc_dmabuf_gl_env})

# MockVideoStreamServer is only compiled into single-config Debug builds (see its directory's CMakeLists.txt).
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(
        SOURCE Receiver/GstVideoReceiverTest.cc TARGET_DIRECTORY QGCUnitTests
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    )
endif()
