set(EXTRA_LIBRARIES)

if (GST_FOUND)
	set(EXTRA_LIBRARIES ${GST_LIBRARIES})
endif()

add_library(VideoStreaming
    GLVideoItemStub.cc
    GLVideoItemStub.h
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoReceiver.cc
    VideoReceiver.h
    VideoStreaming.cc
    VideoStreaming.h
)

target_link_libraries(VideoStreaming
        PRIVATE
                gst_plugins_good
	PUBLIC
		qgc
		Qt5::Multimedia
		Qt5::OpenGL
		${EXTRA_LIBRARIES}
)

target_include_directories(VideoStreaming INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
