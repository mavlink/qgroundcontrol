target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GstVideoItemStub.h)
    return()
endif()

if(WIN32)
    add_subdirectory(gstqml6d3d11)
else()
    add_subdirectory(gstqml6gl)
endif()

if(TARGET gstqml6gl)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6gl)
elseif(TARGET gstqml6d3d11)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6d3d11)
else()
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamer.cc
        GStreamer.h
        GStreamerHelpers.cc
        GStreamerHelpers.h
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

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)

add_subdirectory(gstqgc)
