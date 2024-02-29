set(EXTRA_SOURCES)
set(EXTRA_LIBRARIES)

if (GST_FOUND)
    set(EXTRA_SOURCES
    	gstqgc.c
    	gstqgcvideosinkbin.c
    	GStreamer.cc
    	GStreamer.h
    	GstVideoReceiver.cc
    	GstVideoReceiver.h
    )

    set(EXTRA_LIBRARIES qmlglsink ${GST_LINK_LIBRARIES})
endif()

qt_add_library(VideoReceiver STATIC
    ${EXTRA_SOURCES}
    VideoReceiver.h
)

target_link_libraries(VideoReceiver
    PUBLIC
        Qt6::Multimedia
        Qt6::OpenGL
        Qt6::Quick
        ${EXTRA_LIBRARIES}
        Utilities
)

target_include_directories(VideoReceiver INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if (GST_FOUND)
	target_include_directories(VideoReceiver PUBLIC ${GST_INCLUDE_DIRS})
endif()
