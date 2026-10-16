# QGC tests add sources to ${CMAKE_PROJECT_NAME} (test/Vehicle/, test/MissionManager/); keep these tests on the
# same target.

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GStreamerEnvironmentTest.cc
            GStreamerEnvironmentTest.h
            GStreamerTest.cc
            GStreamerTest.h
            GStreamerTestHelpers.cc
            GStreamerTestHelpers.h
            HwBuffers/GStreamerHwFrameTexturesTest.cc
            HwBuffers/GStreamerHwImportCacheTest.cc
            HwBuffers/common/GStreamerHwBuffersCommonTest.cc
            HwBuffers/d3d/GStreamerD3DTest.cc
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
add_qgc_test(GStreamerTest LABELS Integration)
add_qgc_test(GstVideoReceiverTest LABELS Integration)
add_qgc_test(GstVideoReceiverComponentsTest LABELS Unit)

# MockVideoStreamServer is only compiled into single-config Debug builds (see its directory's CMakeLists.txt).
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(
        SOURCE Receiver/GstVideoReceiverTest.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    )
endif()
