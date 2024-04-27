function(download_gstreamer version)
	# TODO: Verify Version Number
	set(GST_VERSION ${version})

	include(CMakePrintHelpers)

	if(ANDROID)
		FetchContent_Declare(gstreamer
			URL https://gstreamer.freedesktop.org/data/pkg/android/${GST_VERSION}/gstreamer-1.0-android-universal-${GST_VERSION}.tar.xz
			DOWNLOAD_EXTRACT_TIMESTAMP true
		)
		FetchContent_MakeAvailable(gstreamer)
		cmake_print_variables(gstreamer_SOURCE_DIR)

		if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL armeabi-v7a)
            set(GSTREAMER_ROOT ${gstreamer_SOURCE_DIR}/armv7 PARENT_SCOPE)
        elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL arm64-v8a)
            set(GSTREAMER_ROOT ${gstreamer_SOURCE_DIR}/arm64 PARENT_SCOPE)
        elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86)
            set(GSTREAMER_ROOT ${gstreamer_SOURCE_DIR}/x86 PARENT_SCOPE)
        elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86_64)
            set(GSTREAMER_ROOT ${gstreamer_SOURCE_DIR}/x86_64 PARENT_SCOPE)
        endif()
	endif()

	FetchContent_Declare(gstreamer_good_plugins
		URL https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GST_VERSION}.tar.xz
		DOWNLOAD_EXTRACT_TIMESTAMP true
	)
	FetchContent_MakeAvailable(gstreamer_good_plugins)
	cmake_print_variables(gstreamer_good_plugins_SOURCE_DIR)
	set(GST_QT6_PLUGIN_PATH ${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6 PARENT_SCOPE)
endfunction()
