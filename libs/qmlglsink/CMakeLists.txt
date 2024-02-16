option(QGC_ENABLE_VIDEOSTREAMING "Enable video streaming" ON)
if(QGC_ENABLE_VIDEOSTREAMING)
	message(STATUS "Enabling video streaming support")

	find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)
	qt_add_library(qmlglsink STATIC)
	target_link_libraries(qmlglsink
            PUBLIC
		Qt6::Core
		Qt6::Gui
		Qt6::GuiPrivate
		Qt6::OpenGL
		Qt6::Qml
		Qt6::Quick
	)
	target_compile_options(qmlglsink
		PRIVATE
			-Wno-unused-parameter
			-Wno-implicit-fallthrough
			-Wno-unused-private-field
	)

	if(LINUX)
		find_package(PkgConfig)
                pkg_check_modules(gstreamer
			REQUIRED
			IMPORTED_TARGET
			gstreamer-1.0>=1.16
                        gstreamer-video-1.0>=1.16
                        gstreamer-gl-1.0>=1.16
		)
		target_link_libraries(qmlglsink
                    PUBLIC
                        PkgConfig::gstreamer
		)
                if(GST_FOUND)
                        message(STATUS "Building qmlglsink")
                        target_sources(qmlglsink
                                PRIVATE
                                        qt6-linux/gstplugin.cc
                                        qt6-linux/gstqml6glsink.cc
                                        qt6-linux/gstqml6glsink.h
                                        qt6-linux/gstqsg6glnode.cc
                                        qt6-linux/gstqsg6glnode.h
                                        qt6-linux/gstqt6element.cc
                                        qt6-linux/gstqt6elements.h
                                        qt6-linux/gstqt6gl.h
                                        qt6-linux/gstqt6glutility.cc
                                        qt6-linux/gstqt6glutility.h
                                        qt6-linux/qt6glitem.cc
                                        qt6-linux/qt6glitem.h
                        )
                        target_include_directories(qmlglsink PUBLIC qt6-linux)
                        target_compile_definitions(qmlglsink
                                PRIVATE
                                        HAVE_QT_X11
                                        HAVE_QT_WAYLAND
                                        HAVE_QT_EGLFS
                                        HAVE_QT_QPA_HEADER
                                        QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
                                PUBLIC
                                        QGC_GST_STREAMING
                        )
                else()
                        message(STATUS "Skipping qmlglsink - GST Not Found")
                endif()
	elseif(ANDROID)
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
                    set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal/armv7)
                elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
                    set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal/arm64)
                endif()
                message(STATUS ${CMAKE_PREFIX_PATH})
                if(EXISTS ${GSTREAMER_ROOT})
                    list(APPEND CMAKE_PREFIX_PATH ${GSTREAMER_ROOT})
                    list(APPEND PKG_CONFIG_ARGN "--static")
                    pkg_check_modules(GST
                            REQUIRED
                            NO_CMAKE_ENVIRONMENT_PATH
                            IMPORTED_TARGET
                            gstreamer-1.0>=1.22
                            gstreamer-video-1.0
                            gstreamer-gl-1.0
                            gstreamer-rtsp-1.0
                            gstreamer-audio-1.0
                            # gstreamer-plugins-base-1.0
                            gstreamer-codecparsers-1.0
                            gstreamer-rtp-1.0
                            gstreamer-pbutils-1.0
                            gstreamer-net-1.0
                            gstreamer-tag-1.0
                            gstreamer-mpegts-1.0
                            gstreamer-riff-1.0
                            gstreamer-photography-1.0
                            gstreamer-controller-1.0
                            gmodule-2.0
                            graphene-1.0
                            orc-0.4
                            bzip2
                            libffi
                            libpcre2-8
                            zlib
                            # libpng
                            # libjpeg
                            # libavcodec
                            # libavfilter
                            # libavformat
                            # libavutil
                            # x264
                    )
                endif()
                if(GST_FOUND)
                        message(STATUS "Building qmlglsink")
                        target_link_libraries(qmlglsink
                            PUBLIC
                                PkgConfig::GST
                                gstcoreelements
                                gstplayback
                                gstudp
                                gstrtp
                                gstrtsp
                                # gstx264
                                # gstlibav
                                gstsdpelem
                                gstvideoparsersbad
                                gstrtpmanager
                                gstisomp4
                                gstmatroska
                                gstmpegtsdemux
                                gstandroidmedia
                                gstopengl
                                gsttcp
                                iconv
                                # x264
                                z
                                png16
                                jpeg
                                android
                        )
                        target_link_directories(qmlglsink
                                PUBLIC
                                        ${GST_LIBRARY_DIRS}
                                        ${GSTREAMER_ROOT}/lib/gstreamer-1.0
                        )
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
                        target_compile_definitions(qmlglsink
                                PRIVATE
                                        HAVE_QT_ANDROID
                                        HAVE_QT_QPA_HEADER
                                        QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
                                PUBLIC
                                        QGC_GST_STREAMING
                                        QGC_CMAKE_GST
                        )
                        # qt_add_qml_module(qmlglsink
                        #     URI org.freedesktop.gstreamer.qml6
                        #     VERSION 1.0
                        #     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
                        # )
                else()
                        message(STATUS "Skipping qmlglsink - GST Not Found")
                endif()
                message(STATUS "GStreamer found: ${GST_FOUND}")
                message(STATUS "GStreamer libs: ${GST_LIBRARIES}")
                message(STATUS "GStreamer static libs: ${GST_STATIC_LIBRARIES}")
                message(STATUS "GStreamer link libs: ${GST_LINK_LIBRARIES}")
                message(STATUS "GStreamer link dirs: ${GST_LIBRARY_DIRS}")
                message(STATUS "GStreamer ldflags: ${GST_LDFLAGS}")
                message(STATUS "GStreamer ldflags other: ${GST_LDFLAGS_OTHER}")
                message(STATUS "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
                message(STATUS "GStreamer cflags: ${GST_CFLAGS}")
                message(STATUS "GStreamer cflags other: ${GST_CFLAGS_OTHER}")
	endif()
else()
        message(STATUS "Video streaming disabled")
endif()

# 	elseif(MACOS)
# 		target_compile_definitions(qmlglsink
# 			PRIVATE
# 				HAVE_QT_MAC
# 		)
# 	elseif(IOS)
# 		target_compile_definitions(qmlglsink
# 			PRIVATE
# 				HAVE_QT_IOS
# 		)
# 	elseif(WIN32)
# 		target_compile_definitions(qmlglsink
# 			PRIVATE
# 				HAVE_QT_WIN32
# 		)

# 		find_package(OpenGL)
# 		target_link_libraries(qmlglsink
# 			PUBLIC
# 				OpenGL::GL
# 				user32.lib
# 		)

# 		#if(MSVC)
# 		#	target_include_directories(qmlglsink PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/windows)
# 		#endif()
