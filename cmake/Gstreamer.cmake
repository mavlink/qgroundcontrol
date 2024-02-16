set(GST_VERSION "1.22.10")
if(ANDROID)
	set(GST_PATH "${CMAKE_SOURCE_DIR}"/gstreamer-1.0-android-universal.tar.xz)
	if(NOT EXISTS "${GST_PATH}")
		file(DOWNLOAD wget --quiet https://gstreamer.freedesktop.org/data/pkg/android/"{GST_VERSION}"/gstreamer-1.0-android-universal-"{GST_VERSION}".tar.xz "${GST_PATH}")
		execute_process(COMMAND mkdir gstreamer-1.0-android-universal)
		execute_process(COMMAND tar xf gstreamer-1.0-android-universal.tar.xz -C gstreamer-1.0-android-universal)
	endif()
elseif(WIN32)

elseif(UNIX AND APPLE AND NOT IOS)

elseif(UNIX AND APPLE)

elseif(UNIX)

endif()
