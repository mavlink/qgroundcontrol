if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

# qt_add_library(gstqgc STATIC)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        gstqgc.cc
        gstqgcelement.cc
        gstqgcelements.h
        gstqgcvideosinkbin.cc
        gstqgcvideosinkbin.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# find_package(GStreamer REQUIRED)

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::GStreamer)

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
        <gst/gst.h>
)
