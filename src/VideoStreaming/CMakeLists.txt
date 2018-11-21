
set(EXTRA_SRC)
if(VIDEO)
	list(APPEND EXTRA_SRC
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
		gstqtvideosink/painters/genericsurfacepainter.cpp
		gstqtvideosink/painters/openglsurfacepainter.cpp
		gstqtvideosink/painters/videomaterial.cpp
		gstqtvideosink/painters/videonode.cpp
		gstqtvideosink/utils/bufferformat.cpp
		gstqtvideosink/utils/utils.cpp
	)
endif()

add_library(VideoStreaming
	VideoItem.cc
	VideoReceiver.cc
	VideoStreaming.cc
	VideoSurface.cc
	${EXTRA_SRC}
)

target_link_libraries(VideoStreaming
	PUBLIC
		qgc

		Qt5::Multimedia
)

target_include_directories(VideoStreaming INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
