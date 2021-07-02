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

add_library(VideoReceiver
    ${EXTRA_SOURCES}
    VideoReceiver.h
)

target_link_libraries(VideoReceiver
    PUBLIC
        Qt5::Multimedia
        Qt5::OpenGL
        Qt5::Quick
        ${EXTRA_LIBRARIES}
        Settings
)

target_include_directories(VideoReceiver INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if (GST_FOUND)
	target_include_directories(VideoReceiver PUBLIC ${GST_INCLUDE_DIRS})
endif()
