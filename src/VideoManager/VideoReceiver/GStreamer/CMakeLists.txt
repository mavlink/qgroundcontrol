find_package(Qt6 REQUIRED COMPONENTS Core Quick)

target_sources(QGC PRIVATE GLVideoItemStub.h)

target_link_libraries(QGC PUBLIC Qt6::Quick)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

add_subdirectory(gstqml6gl)

if(NOT TARGET gstqml6gl)
    return()
endif()

target_sources(QGC
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
    target_sources(QGC
        PRIVATE
            gst_ios_init.m
            gst_ios_init.h
    )
endif()

target_link_libraries(QGC
    PRIVATE
        gstqml6gl
    PUBLIC
        Qt6::Core
)

target_compile_definitions(QGC PUBLIC QGC_GST_STREAMING)
