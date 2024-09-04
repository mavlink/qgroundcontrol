find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_add_library(GStreamerReceiver STATIC GLVideoItemStub.h)
target_link_libraries(GStreamerReceiver PUBLIC Qt6::Quick)
target_include_directories(GStreamerReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer Video Streaming Backend" ON)
cmake_print_variables(QGC_ENABLE_GST_VIDEOSTREAMING)
if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

add_subdirectory(qmlglsink)

if(NOT GST_QT6_PLUGIN_FOUND)
    return()
endif()

target_sources(GStreamerReceiver
    PRIVATE
        gstqgc.c
        gstqgcvideosinkbin.c
        GStreamer.cc
        GStreamer.h
        GstVideoReceiver.cc
        GstVideoReceiver.h
)

if(IOS)
    target_sources(GStreamerReceiver
        PRIVATE
            gst_ios_init.h
            gst_ios_init.m
    )
endif()

target_link_libraries(GStreamerReceiver
    PRIVATE
        Utilities
    PUBLIC
        Qt6::Core
        qmlglsink
        Settings
        VideoReceiver
)

target_compile_definitions(GStreamerReceiver PUBLIC QGC_GST_STREAMING)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(GStreamerReceiver PRIVATE QGC_INSTALL_RELEASE)
endif()
