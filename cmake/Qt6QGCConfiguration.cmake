if(DEFINED ENV{QT_VERSION})
	set(QT_VERSION $ENV{QT_VERSION})
endif()

if(NOT QT_VERSION)
	# if QT version not specified then use any available version
	file(GLOB FOUND_QT_VERSIONS
		LIST_DIRECTORIES true
		$ENV{HOME}/Qt/6.6.*
	)
	if(NOT FOUND_QT_VERSIONS)
		return()
	endif()
	list(GET FOUND_QT_VERSIONS 0 QT_VERSION_PATH)
	get_filename_component(QT_VERSION ${QT_VERSION_PATH} NAME)
endif()

if(DEFINED ENV{QT_MKSPEC})
	set(QT_MKSPEC $ENV{QT_MKSPEC})
endif()

if(NOT QT_MKSPEC)
	if(APPLE)
		set(QT_MKSPEC clang_64)
	elseif(LINUX)
		set(QT_MKSPEC gcc_64)
	elseif(WIN32)
		set(QT_MKSPEC msvc2019_64)
	elseif(ANDROID)
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
			set(QT_MKSPEC android_armv7)
		elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
			set(QT_MKSPEC android_arm64_v8a)
		elseif(${ANDROID_ABI} STREQUAL x86)
			set(QT_MKSPEC android_x86)
		elseif(${ANDROID_ABI} STREQUAL x86_64)
			set(QT_MKSPEC android_x86_64)
		endif()
	endif()
endif()

set(QT_LIBRARY_HINTS
	$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
	${Qt6_DIR}
)

if(ANDROID)
	list(APPEND QT_LIBRARY_HINTS ${QT_HOST_PATH}/lib/cmake)
elseif(WIN32)
	list(APPEND QT_LIBRARY_HINTS C:/Qt/${QT_VERSION}/${QT_MKSPEC})
elseif(LINUX)
	list(APPEND QT_LIBRARY_HINTS $ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC})
endif()

include(CMakePrintHelpers)
cmake_print_variables(QT_VERSION QT_MKSPEC QT_LIBRARY_HINTS)
