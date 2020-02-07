set(EXTRA_LIBRARIES)

if (GST_FOUND)
	set(EXTRA_LIBRARIES ${GST_LIBRARIES})
endif()

add_library(VideoStreaming
	VideoReceiver.cc
	VideoStreaming.cc
	SubtitleWriter.cc
  GLVideoItemStub.cc
)

target_link_libraries(VideoStreaming
	PUBLIC
		qgc
		Qt5::Multimedia
		Qt5::OpenGL
		${EXTRA_LIBRARIES}
)

target_include_directories(VideoStreaming INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
