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
		set(QT_MKSPEC msvc2017_64)
	elseif(ANDROID)
		if(${ANDROID_ABI} STREQUAL armeabi-v7a)
			set(QT_MKSPEC android_armv7)
		elseif(${ANDROID_ABI} STREQUAL arm64-v8a)
			set(QT_MKSPEC android_arm64_v8a)
		endif()
	endif()
endif()

set(QT_LIBRARY_HINTS
		$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
		$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
		C:/Qt
)

message(STATUS "lib hints ${QT_LIBRARY_HINTS}")