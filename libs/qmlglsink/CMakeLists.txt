option(QGC_ENABLE_VIDEOSTREAMING "Enable video streaming" ON)
if(QGC_ENABLE_VIDEOSTREAMING)
	message(STATUS "Enabling video streaming support")

	if(ANDROID)
		set(GST_STATIC_BUILD ON)
	else()
		set(GST_STATIC_BUILD OFF)
	endif()

    if(LINUX)
	    set(GST_TARGET_VERSION 1.16)
	elseif(ANDROID)
		set(GST_TARGET_VERSION 1.18.5)
	else()
		set(GST_TARGET_VERSION 1.18)
	endif()

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
		list(APPEND GST_TARGET_MODULES egl)
	endif()

	# find_library(GSTREAMER_${_upper_name}_LIBRARY
    #      NAMES
    #      	gst${_lower_name}-${_abi_version}
    #      HINTS
    #      	${PKG_GSTREAMER_${_upper_name}_LIBRARY_DIRS}
    #         ${PKG_GSTREAMER_${_upper_name}_LIBDIR}
    # )

    # find_path(GSTREAMER_${_upper_name}_INCLUDE_DIR
	# 	gst/${_lower_name}/${_header}
	# 	HINTS
	# 		${PKG_GSTREAMER_${_upper_name}_INCLUDE_DIRS}
	# 		PKG_GSTREAMER_${_upper_name}_INCLUDEDIR}
	# 	PATH_SUFFIXES gstreamer-${_abi_version}
    # )

	set(GSTREAMER_ROOT)
	if(WIN32)
		if(EXISTS $ENV{GSTREAMER_ROOT_X86_64})
			set(GSTREAMER_ROOT $ENV{GSTREAMER_ROOT_X86_64})
		elseif(EXISTS $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
			set(GSTREAMER_ROOT $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
		else()
			set(GSTREAMER_ROOT "C:/gstreamer/1.0/msvc_x86_64")
		endif()
	elseif(MACOS)
		set(GSTREAMER_ROOT "/Library/Frameworks/GStreamer.framework")
	elseif(ANDROID)
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
            set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal-${GST_TARGET_VERSION}/armv7)
        elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
            set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal-${GST_TARGET_VERSION}/arm64)
        elseif(${ANDROID_ABI} STREQUAL x86)
            set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal-${GST_TARGET_VERSION}/x86)
        elseif(${ANDROID_ABI} STREQUAL x86_64)
            set(GSTREAMER_ROOT ${CMAKE_SOURCE_DIR}/gstreamer-1.0-android-universal-${GST_TARGET_VERSION}/x86_64)
        endif()
	endif()

	find_package(PkgConfig)
	if(NOT EXISTS ${GSTREAMER_ROOT})
		pkg_check_modules(GSTREAMER gstreamer-1.0>=${GST_TARGET_VERSION})
		if(GSTREAMER_FOUND)
			set(GSTREAMER_ROOT ${GSTREAMER_PREFIX})
		endif()
	endif()

	if(EXISTS ${GSTREAMER_ROOT})
		message(STATUS "Gstreamer found")
		cmake_print_variables(GSTREAMER_ROOT)

		find_package(Qt6 REQUIRED COMPONENTS Core)
		qt_add_library(qmlglsink STATIC)

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
			if(GST_STATIC_BUILD)
		        target_link_libraries(qmlglsink PUBLIC ${GST_STATIC_LINK_LIBRARIES})
	    		target_link_directories(qmlglsink PUBLIC ${GST_STATIC_LIBRARY_DIRS})
		        target_link_options(qmlglsink PUBLIC ${GST_STATIC_LDFLAGS} ${GST_STATIC_LDFLAGS_OTHER})
		        target_compile_options(qmlglsink PUBLIC ${GST_STATIC_CFLAGS} ${GST_STATIC_CFLAGS_OTHER})
	    		target_include_directories(qmlglsink PUBLIC ${GST_STATIC_INCLUDE_DIRS})
	    		if(ANDROID)
					target_link_options(PkgConfig::GST INTERFACE "-Wl,-Bsymbolic")
				endif()
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
	    	endif()
	    	# file(GET_RUNTIME_DEPENDENCIES)
		endif()
	else()
		message(WARNING "Gstreamer Not Found")
    endif()

	if(GST_FOUND)
		message(STATUS "GST Modules Found")
		target_sources(qmlglsink
	        PRIVATE
	            qt6/gstplugin.cc
	            qt6/gstqml6glsink.cc
	            qt6/gstqml6glsink.h
	            qt6/gstqsg6glnode.cc
	            qt6/gstqsg6glnode.h
	            qt6/gstqt6element.cc
	            qt6/gstqt6elements.h
	            qt6/gstqt6gl.h
	            qt6/gstqt6glutility.cc
	            qt6/gstqt6glutility.h
	            qt6/qt6glitem.cc
	            qt6/qt6glitem.h
	    )

		find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)
	    target_link_libraries(qmlglsink
	        PUBLIC
				Qt6::Core
				Qt6::Gui
				Qt6::GuiPrivate
				Qt6::OpenGL
				Qt6::Qml
				Qt6::Quick
		)
		if(WIN32)
			find_package(OpenGL)
			target_link_libraries(qmlglsink PUBLIC OpenGL::GL)
		elseif(LINUX)
			# find_package(Qt6 COMPONENTS WaylandClient)
			if(Qt6WaylandClient_FOUND)
				target_link_libraries(qmlglsink PRIVATE Qt6::WaylandClient)
			endif()
		endif()

	    target_include_directories(qmlglsink PUBLIC qt6)

	    target_compile_definitions(qmlglsink
	        PRIVATE
	            HAVE_QT_QPA_HEADER
	            QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
	    )
	    if(LINUX)
	    	target_compile_definitions(qmlglsink PRIVATE HAVE_QT_X11)
	    	if(EGL_FOUND)
				target_compile_definitions(qmlglsink PRIVATE HAVE_QT_EGLFS)
			endif()
	        if(Qt6WaylandClient_FOUND)
				target_compile_definitions(qmlglsink PRIVATE HAVE_QT_WAYLAND)
			endif()
		elseif(ANDROID)
			target_compile_definitions(qmlglsink PRIVATE HAVE_QT_ANDROID)
		elseif(WIN32)
			target_compile_definitions(qmlglsink PRIVATE HAVE_QT_WIN32)
	    elseif(MACOS)
	    	target_compile_definitions(qmlglsink PRIVATE HAVE_QT_MAC)
	    elseif(IOS)
			target_compile_definitions(qmlglsink PRIVATE HAVE_QT_IOS)
			message(WARNING "qmlglsink not supported for IOS")
	   	endif()

	   	if(UNIX)
	   		target_compile_options(qmlglsink
	            PRIVATE
	                -Wno-unused-parameter
	                -Wno-implicit-fallthrough
	                -Wno-unused-private-field
	        )
	    endif()

	    message(STATUS "GStreamer version: ${GST_gstreamer-1.0_VERSION}")
	    message(STATUS "GStreamer prefix: ${GST_gstreamer-1.0_PREFIX}")
	    message(STATUS "GStreamer include dir: ${GST_gstreamer-1.0_INCLUDEDIR}")
	    message(STATUS "GStreamer libdir: ${GST_gstreamer-1.0_LIBDIR}")
		if(GST_STATIC_BUILD)
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
	else()
		message(WARNING "GST Modules Not Found")
    endif()
else()
    message(STATUS "Video streaming disabled")
endif()
