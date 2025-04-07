target_sources(${CMAKE_PROJECT_NAME} PRIVATE GLVideoItemStub.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

add_subdirectory(gstqml6gl)

if(NOT TARGET gstqml6gl)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        gstqgc.cc
        gstqgcelement.cc
        gstqgcelements.h
        gstqgcvideosinkbin.cc
        gstqgcvideosinkbin.h
        GStreamer.cc
        GStreamer.h
        GstVideoReceiver.cc
        GstVideoReceiver.h
)

if(IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            gst_ios_init.m
            gst_ios_init.h
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6gl)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)
