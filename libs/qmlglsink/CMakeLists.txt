option(QGC_ENABLE_VIDEOSTREAMING "Enable video streaming" ON)
if(QGC_ENABLE_VIDEOSTREAMING)
	message(STATUS "Enabling video streaming support")

	find_package(PkgConfig)

	if(LINUX)
		set(GST_DEPENDENCIES
			gstreamer-1.0>=1.16
			gstreamer-video-1.0>=1.16
			gstreamer-gl-1.0>=1.16
			egl
		)
	else()
		set(GST_DEPENDENCIES
			gstreamer-1.0>=1.22
			gstreamer-video-1.0>=1.22
			gstreamer-gl-1.0>=1.22
		)
	endif()

	if(ANDROID)
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
			set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal/armv7)
		elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
			set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal/arm64)
		endif()
		if(EXISTS ${GSTREAMER_ROOT})
			set(GST_FOUND TRUE)
		endif()
	else()
		pkg_check_modules(GST ${GST_DEPENDENCIES})
	endif()

	if(NOT ANDROID)
		message(STATUS "GStreamer found: ${GST_FOUND}")
		message(STATUS "GStreamer libs: ${GST_LIBRARIES}")
		message(STATUS "GStreamer link libs: ${GST_LINK_LIBRARIES}")
		message(STATUS "GStreamer link dirs: ${GST_LIBRARY_DIRS}")
		message(STATUS "GStreamer ldflags: ${GST_LDFLAGS}")
		message(STATUS "GStreamer ldflags other: ${GST_LDFLAGS_OTHER}")
		message(STATUS "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
		message(STATUS "GStreamer cflags: ${GST_CFLAGS}")
		message(STATUS "GStreamer cflags other: ${GST_CFLAGS_OTHER}")
	endif()

	if(GST_FOUND)
		message(STATUS "Building qmlglsink")

		find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)
		qt_add_library(qmlglsink STATIC)

		if(LINUX)
			target_sources(qmlglsink
				PRIVATE
					gst-plugins-good/ext/qt6/gstplugin.cc
					gst-plugins-good/ext/qt6/gstqml6glsink.cc
					gst-plugins-good/ext/qt6/gstqml6glsink.h
					gst-plugins-good/ext/qt6/gstqsg6glnode.cc
					gst-plugins-good/ext/qt6/gstqsg6glnode.h
					gst-plugins-good/ext/qt6/gstqt6element.cc
					gst-plugins-good/ext/qt6/gstqt6elements.h
					gst-plugins-good/ext/qt6/gstqt6gl.h
					gst-plugins-good/ext/qt6/gstqt6glutility.cc
					gst-plugins-good/ext/qt6/gstqt6glutility.h
					gst-plugins-good/ext/qt6/qt6glitem.cc
					gst-plugins-good/ext/qt6/qt6glitem.h
			)
			target_include_directories(qmlglsink PUBLIC gst-plugins-good/ext/qt6)
		else()
			target_sources(qmlglsink
				PRIVATE
					qt6/gstplugin.cc
					qt6/gstqml6glmixer.cc
					qt6/gstqml6glmixer.h
					qt6/gstqml6gloverlay.cc
					qt6/gstqml6gloverlay.h
					qt6/gstqml6glsink.cc
					qt6/gstqml6glsink.h
					qt6/gstqml6glsrc.cc
					qt6/gstqml6glsrc.h
					qt6/gstqsg6material.cc
					qt6/gstqsg6material.h
					qt6/gstqt6element.cc
					qt6/gstqt6elements.h
					qt6/gstqt6gl.h
					qt6/gstqt6glutility.cc
					qt6/gstqt6glutility.h
					qt6/qt6glitem.cc
					qt6/qt6glitem.h
					qt6/qt6glrenderer.cc
					qt6/qt6glrenderer.h
					qt6/qt6glwindow.cc
					qt6/qt6glwindow.h
			)
			target_include_directories(qmlglsink PUBLIC qt6)
			qt_add_resources(qmlglsink "gst"
				PREFIX "/org/freedesktop/gstreamer/qml6"
				FILES
					qt6/RGBA.frag
					qt6/vertex.vert
					qt6/YUV_TRIPLANAR.frag
			)
		endif()

		if(LINUX)
			target_compile_definitions(qmlglsink
				PRIVATE
					HAVE_QT_X11
					HAVE_QT_WAYLAND
					HAVE_QT_EGLFS
			)
		elseif(MACOS)
			target_compile_definitions(qmlglsink
				PRIVATE
					HAVE_QT_MAC
			)
		elseif(IOS)
			target_compile_definitions(qmlglsink
				PRIVATE
					HAVE_QT_IOS
			)
		elseif(WIN32)
			target_compile_definitions(qmlglsink
				PRIVATE
					HAVE_QT_WIN32
			)

			find_package(OpenGL)
			target_link_libraries(qmlglsink
				PUBLIC
					OpenGL::GL
					user32.lib
			)

			#if(MSVC)
			#	target_include_directories(qmlglsink PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/windows)
			#endif()
		elseif(ANDROID)
			target_compile_definitions(qmlglsink
				PRIVATE
					HAVE_QT_ANDROID
			)
		endif()

		target_compile_definitions(qmlglsink
			PRIVATE
				HAVE_QT_QPA_HEADER
				QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
			PUBLIC
				QGC_GST_STREAMING
		)

		target_link_libraries(qmlglsink
			PUBLIC
				Qt6::Core
				Qt6::Gui
				Qt6::GuiPrivate
				Qt6::OpenGL
				Qt6::Qml
				Qt6::Quick
		)

		if(ANDROID)
			SET(GST_LIBS
				graphene-1.0 jpeg png16
				gstfft-1.0 m
				gstnet-1.0 gio-2.0
				gstphotography-1.0 gstgl-1.0 EGL
				gstaudio-1.0 gstcodecparsers-1.0 gstbase-1.0
				gstreamer-1.0 gstrtp-1.0 gstpbutils-1.0 gstrtsp-1.0 gsttag-1.0
				gstvideo-1.0 avformat avcodec avutil x264 avfilter swresample
				gstriff-1.0 gstcontroller-1.0 gstapp-1.0
				gstsdp-1.0 bz2 gobject-2.0 gstmpegts-1.0
				gmodule-2.0 pthread glib-2.0 orc-0.4 iconv ffi intl
			)
			LIST(APPEND GST_LIBS
				gstcoreelements
				gstplayback
				gstudp
				gstrtp
				gstrtsp
				gstx264
				gstlibav
				gstsdpelem
				gstvideoparsersbad
				gstrtpmanager
				gstisomp4
				gstmatroska
				gstmpegtsdemux
				gstandroidmedia
				gstopengl
				gsttcp
			)
			target_link_libraries(qmlglsink
				PUBLIC
					${GST_LIBS}
			)

			target_include_directories(qmlglsink
				PUBLIC
					${GSTREAMER_ROOT}/include
					${GSTREAMER_ROOT}/include/gstreamer-1.0
					${GSTREAMER_ROOT}/lib/gstreamer-1.0/include
					${GSTREAMER_ROOT}/include/glib-2.0
					${GSTREAMER_ROOT}/lib/glib-2.0/include
			)

			target_link_directories(qmlglsink
				PUBLIC
					${GSTREAMER_ROOT}/lib
					${GSTREAMER_ROOT}/lib/gstreamer-1.0
			)
		else()
			target_link_libraries(qmlglsink
				PUBLIC
					${GST_LINK_LIBRARIES}
			)
			target_include_directories(qmlglsink PUBLIC ${GST_INCLUDE_DIRS})
		endif()

		target_compile_options(qmlglsink
			PRIVATE
				-Wno-unused-parameter
				-Wno-implicit-fallthrough
				-Wno-unused-private-field
		)
	else()
		message(STATUS "Skipping qmlglsink - GST Not Found")
	endif()
else()
	message(STATUS "Video streaming disabled")
endif()
