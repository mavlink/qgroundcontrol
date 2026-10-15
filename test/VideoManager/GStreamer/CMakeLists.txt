# QGC tests add sources to ${CMAKE_PROJECT_NAME} (test/Vehicle/, test/MissionManager/); keep this test on the same target.

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamerTest.cc
        GStreamerTest.h
        HwBuffers/GStreamerHwFrameTexturesTest.cc
        HwBuffers/GStreamerHwImportCacheTest.cc
        HwBuffers/common/GStreamerHwBuffersCommonTest.cc
        HwBuffers/d3d/GStreamerD3DTest.cc
        HwBuffers/dmabuf/GStreamerDmaBufTest.cc
        HwBuffers/gl/GStreamerGlTest.cc
        HwBuffers/vulkan/GStreamerVulkanTest.cc
        Receiver/GstVideoReceiverTest.cc
        Receiver/GstVideoReceiverTest.h
        SourceFactory/GStreamerSourceFactoryTest.cc
        gstqgc/GStreamerGstQgcTest.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GStreamerTest LABELS Integration)
add_qgc_test(GstVideoReceiverTest LABELS Integration)
