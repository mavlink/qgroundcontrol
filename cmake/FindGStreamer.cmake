if(ANDROID)
	set(GST_STATIC_BUILD ON)
else()
	set(GST_STATIC_BUILD OFF)
endif()

if(DEFINED ENV{GST_VERSION})
	set(GST_TARGET_VERSION $ENV{GST_VERSION})
elseif(LINUX)
	set(GST_TARGET_VERSION 1.16)
else()
	set(GST_TARGET_VERSION 1.22.11)
endif()
# TODO: Download using ${GST_gstreamer-1.0_VERSION}

include(DownloadGStreamer)
download_gstreamer(${GST_TARGET_VERSION})

set(GST_TARGET_PLUGINS
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
    gstopengl
    gsttcp
)
if(ANDROID)
	list(APPEND GST_TARGET_PLUGINS gstandroidmedia)
elseif(IOS)
	list(APPEND GST_TARGET_PLUGINS gstapplemedia)
endif()

set(GST_TARGET_MODULES
	gstreamer-1.0
	gstreamer-gl-1.0
	gstreamer-video-1.0
)
if(LINUX)
	list(APPEND GST_TARGET_MODULES
		# gstreamer-gl-x11-1.0
		# gstreamer-gl-egl-1.0
		# gstreamer-gl-wayland-1.0
		egl
	)
endif()

if(NOT GSTREAMER_ROOT)
	set(GSTREAMER_ROOT)
	if(WIN32)
		if(DEFINED ENV{GSTREAMER_ROOT_X86_64} AND EXISTS $ENV{GSTREAMER_ROOT_X86_64})
			set(GSTREAMER_ROOT $ENV{GSTREAMER_ROOT_X86_64})
		elseif(DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64} AND EXISTS $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
			set(GSTREAMER_ROOT $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
		else()
			set(GSTREAMER_ROOT "C:/gstreamer/1.0/msvc_x86_64")
		endif()
	elseif(MACOS)
		set(GSTREAMER_ROOT "/Library/Frameworks/GStreamer.framework")
	elseif(ANDROID)
		set(ANDROID_GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal-${GST_TARGET_VERSION})
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
            set(GSTREAMER_ROOT ${ANDROID_GSTREAMER_ROOT}/armv7)
        elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
            set(GSTREAMER_ROOT ${ANDROID_GSTREAMER_ROOT}/arm64)
        elseif(${ANDROID_ABI} STREQUAL x86)
            set(GSTREAMER_ROOT ${ANDROID_GSTREAMER_ROOT}/x86)
        elseif(${ANDROID_ABI} STREQUAL x86_64)
            set(GSTREAMER_ROOT ${ANDROID_GSTREAMER_ROOT}/x86_64)
        endif()
	endif()
	cmake_print_variables(GSTREAMER_ROOT)
endif()

find_package(PkgConfig)
if(PkgConfig_FOUND)
	if(NOT EXISTS ${GSTREAMER_ROOT})
		pkg_check_modules(GSTREAMER gstreamer-1.0>=${GST_TARGET_VERSION})
		if(GSTREAMER_FOUND)
			set(GSTREAMER_ROOT ${GSTREAMER_PREFIX})
		endif()
	endif()

	if(EXISTS ${GSTREAMER_ROOT})
		message(STATUS "Gstreamer found")
		cmake_print_variables(GSTREAMER_ROOT)

		if(GST_STATIC_BUILD)
			list(APPEND PKG_CONFIG_ARGN --static)
		endif()

		list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_ROOT})
		if(ANDROID)
			set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_ROOT}/lib/pkgconfig:${GSTREAMER_ROOT}/lib/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
			list(APPEND PKG_CONFIG_ARGN
				--define-prefix
				--define-variable=prefix=${GSTREAMER_ROOT}
				--define-variable=libdir=${GSTREAMER_ROOT}/lib
				--define-variable=includedir=${GSTREAMER_ROOT}/include
			)
			pkg_check_modules(GST
				IMPORTED_TARGET
				NO_CMAKE_ENVIRONMENT_PATH
				${GST_TARGET_MODULES}
				${GST_TARGET_PLUGINS}
			)
		elseif(LINUX)
			set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_ROOT}/lib/pkgconfig:${GSTREAMER_ROOT}/x86_64-linux-gnu/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
			pkg_check_modules(GST IMPORTED_TARGET ${GST_TARGET_MODULES})
		elseif(MACOS)
			set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_ROOT}/Versions/Current/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
			pkg_check_modules(GST IMPORTED_TARGET ${GST_TARGET_MODULES})
		else()
			set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_ROOT}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
			pkg_check_modules(GST IMPORTED_TARGET ${GST_TARGET_MODULES})
		endif()

		if(TARGET PkgConfig::GST)
			target_link_libraries(qmlglsink PUBLIC PkgConfig::GST)
			target_include_directories(qmlglsink PUBLIC ${GSTREAMER_ROOT}/include/gstreamer-1.0)
			message(STATUS "GStreamer version: ${GST_gstreamer-1.0_VERSION}")
		    message(STATUS "GStreamer prefix: ${GST_gstreamer-1.0_PREFIX}")
		    message(STATUS "GStreamer include dir: ${GST_gstreamer-1.0_INCLUDEDIR}")
		    message(STATUS "GStreamer libdir: ${GST_gstreamer-1.0_LIBDIR}")
			if(GST_STATIC_BUILD)
		        target_link_libraries(qmlglsink PUBLIC ${GST_STATIC_LINK_LIBRARIES})
	    		target_link_directories(qmlglsink PUBLIC ${GST_STATIC_LIBRARY_DIRS})
		        target_link_options(qmlglsink PUBLIC ${GST_STATIC_LDFLAGS} ${GST_STATIC_LDFLAGS_OTHER})
		        target_compile_options(qmlglsink PUBLIC ${GST_STATIC_CFLAGS} ${GST_STATIC_CFLAGS_OTHER})
	    		target_include_directories(qmlglsink PUBLIC ${GST_STATIC_INCLUDE_DIRS})
	    		if(ANDROID)
					target_link_options(PkgConfig::GST INTERFACE "-Wl,-Bsymbolic")
				endif()
				list(REMOVE_DUPLICATES GST_STATIC_LIBRARIES)
				list(REMOVE_DUPLICATES GST_STATIC_LINK_LIBRARIES)
				list(REMOVE_DUPLICATES GST_STATIC_LIBRARY_DIRS)
				list(REMOVE_DUPLICATES GST_STATIC_INCLUDE_DIRS)
				list(REMOVE_DUPLICATES GST_STATIC_LDFLAGS)
				list(REMOVE_DUPLICATES GST_STATIC_LDFLAGS_OTHER)
				list(REMOVE_DUPLICATES GST_STATIC_CFLAGS)
				list(REMOVE_DUPLICATES GST_STATIC_CFLAGS_OTHER)
			    message(VERBOSE "GStreamer static libs: ${GST_STATIC_LIBRARIES}")
			    message(VERBOSE "GStreamer static link libs: ${GST_STATIC_LINK_LIBRARIES}")
			    message(VERBOSE "GStreamer static link dirs: ${GST_STATIC_LIBRARY_DIRS}")
			    message(VERBOSE "GStreamer static include dirs: ${GST_STATIC_INCLUDE_DIRS}")
			    message(VERBOSE "GStreamer static ldflags: ${GST_STATIC_LDFLAGS}")
			    message(VERBOSE "GStreamer static ldflags other: ${GST_STATIC_LDFLAGS_OTHER}")
			    message(VERBOSE "GStreamer static cflags: ${GST_STATIC_CFLAGS}")
			    message(VERBOSE "GStreamer static cflags other: ${GST_STATIC_CFLAGS_OTHER}")
	    	else()
		        target_link_libraries(qmlglsink PUBLIC ${GST_LINK_LIBRARIES})
	    		target_link_directories(qmlglsink PUBLIC ${GST_LIBRARY_DIRS})
		        target_link_options(qmlglsink PUBLIC ${GST_LDFLAGS} ${GST_LDFLAGS_OTHER})
		        target_compile_options(qmlglsink PUBLIC ${GST_CFLAGS} ${GST_CFLAGS_OTHER})
	    		target_include_directories(qmlglsink PUBLIC ${GST_INCLUDE_DIRS})
	    		if(WIN32)
	    			cmake_path(CONVERT "${GSTREAMER_ROOT}/bin/*.dll" TO_CMAKE_PATH_LIST GST_WIN_BINS_PATH)
	    			file(GLOB GST_WIN_BINS ${GST_WIN_BINS_PATH})
		    		cmake_print_variables(GST_WIN_BINS_PATH GST_WIN_BINS)
		    		# TODO: Only install needed libs
		    		install(FILES ${GST_WIN_BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
		    	endif()
		    	list(REMOVE_DUPLICATES GST_LIBRARIES)
				list(REMOVE_DUPLICATES GST_LINK_LIBRARIES)
				list(REMOVE_DUPLICATES GST_LIBRARY_DIRS)
				list(REMOVE_DUPLICATES GST_LDFLAGS)
				list(REMOVE_DUPLICATES GST_LDFLAGS_OTHER)
				list(REMOVE_DUPLICATES GST_INCLUDE_DIRS)
				list(REMOVE_DUPLICATES GST_CFLAGS)
				list(REMOVE_DUPLICATES GST_CFLAGS_OTHER)
			    message(VERBOSE "GStreamer libs: ${GST_LIBRARIES}")
			    message(VERBOSE "GStreamer link libs: ${GST_LINK_LIBRARIES}")
			    message(VERBOSE "GStreamer link dirs: ${GST_LIBRARY_DIRS}")
			    message(VERBOSE "GStreamer ldflags: ${GST_LDFLAGS}")
			    message(VERBOSE "GStreamer ldflags other: ${GST_LDFLAGS_OTHER}")
			    message(VERBOSE "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
			    message(VERBOSE "GStreamer cflags: ${GST_CFLAGS}")
			    message(VERBOSE "GStreamer cflags other: ${GST_CFLAGS_OTHER}")
	    	endif()
		endif()
	else()
		message(WARNING "Gstreamer Not Found")
    endif()
else()
	find_library(GSTREAMER NAMES gstreamer-1.0 HINTS ${GSTREAMER_ROOT}/lib)
	if(GSTREAMER)
        target_link_libraries(qmlglsink
        	PUBLIC
        		${GSTREAMER_ROOT}/lib/gstreamer-1.0.lib
        		${GSTREAMER_ROOT}/lib/gstgl-1.0.lib
        		${GSTREAMER_ROOT}/lib/gstvideo-1.0.lib
        		${GSTREAMER_ROOT}/lib/gstbase-1.0.lib
        		${GSTREAMER_ROOT}/lib/glib-2.0.lib
        		${GSTREAMER_ROOT}/lib/intl.lib
        		${GSTREAMER_ROOT}/lib/gobject-2.0.lib
        )

		# find_path(GSTREAMER_INCLUDE_DIR gst/gst.h)
		target_include_directories(qmlglsink
			PUBLIC
				${GSTREAMER_ROOT}/include
				${GSTREAMER_ROOT}/include/gstreamer-1.0
				${GSTREAMER_ROOT}/include/glib-2.0
				${GSTREAMER_ROOT}/lib/gstreamer-1.0/include
				${GSTREAMER_ROOT}/lib/glib-2.0/include
		)

		# TODO: Only install needed libs

		cmake_path(CONVERT "${GSTREAMER_ROOT}/bin/*.dll" TO_CMAKE_PATH_LIST GST_WIN_BINS_PATH)
		file(GLOB GST_WIN_BINS ${GST_WIN_BINS_PATH})
		cmake_print_variables(GST_WIN_BINS_PATH GST_WIN_BINS)
		install(FILES ${GST_WIN_BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})

		cmake_path(CONVERT "${GSTREAMER_ROOT}/lib/gstreamer-1.0/*.dll" TO_CMAKE_PATH_LIST GST_WIN_LIBS_PATH)
		file(GLOB GST_WIN_LIBS ${GST_WIN_LIBS_PATH})
		cmake_print_variables(GST_WIN_LIBS_PATH GST_WIN_LIBS)
		install(FILES ${GST_WIN_LIBS} DESTINATION ${CMAKE_INSTALL_LIBDIR})

		set(GST_FOUND TRUE)
	endif()
endif()
