find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)

qt_add_library(VideoReceiver STATIC
    VideoReceiver.h
)

target_link_libraries(VideoReceiver
    PUBLIC
        Qt6::Multimedia
        Qt6::OpenGL
        Qt6::Quick
        Utilities
)

target_include_directories(VideoReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(GST_FOUND)
    target_sources(VideoReceiver
        PRIVATE
            gstqgc.c
            gstqgcvideosinkbin.c
            GStreamer.cc
            GStreamer.h
            GstVideoReceiver.cc
            GstVideoReceiver.h
    )

    target_link_libraries(VideoReceiver
        PUBLIC
            qmlglsink
            Settings
    )
endif()
