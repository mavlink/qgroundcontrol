set(EXTRA_LIBRARIES)

if (GST_FOUND)
	include_directories(
		gstqtvideosink/utils/
	)

	set(EXTRA_SRC
		gstqtvideosink/delegates/basedelegate.cpp
		gstqtvideosink/delegates/qtquick2videosinkdelegate.cpp
		gstqtvideosink/delegates/qtvideosinkdelegate.cpp
		gstqtvideosink/delegates/qwidgetvideosinkdelegate.cpp
		gstqtvideosink/gstqtglvideosink.cpp
		gstqtvideosink/gstqtglvideosinkbase.cpp
		gstqtvideosink/gstqtquick2videosink.cpp
		gstqtvideosink/gstqtvideosink.cpp
		gstqtvideosink/gstqtvideosinkbase.cpp
		gstqtvideosink/gstqtvideosinkplugin.cpp
		gstqtvideosink/gstqwidgetvideosink.cpp
		gstqtvideosink/gstqtvideosinkmarshal.c
		gstqtvideosink/painters/genericsurfacepainter.cpp
		gstqtvideosink/painters/openglsurfacepainter.cpp
		gstqtvideosink/painters/videomaterial.cpp
		gstqtvideosink/painters/videonode.cpp
		gstqtvideosink/utils/bufferformat.cpp
		gstqtvideosink/utils/utils.cpp
	)
	set(EXTRA_LIBRARIES ${GST_LIBRARIES})
endif()

add_library(VideoStreaming
	VideoItem.cc
	VideoReceiver.cc
	VideoStreaming.cc
	VideoSurface.cc
	SubtitleWriter.cc
	${EXTRA_SRC}
)

target_link_libraries(VideoStreaming
	PUBLIC
		qgc
		Qt5::Multimedia
		Qt5::OpenGL
		${EXTRA_LIBRARIES}
)

target_include_directories(VideoStreaming INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
