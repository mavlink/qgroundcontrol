if(DEFINED ENV{QT_VERSION})
	set(QT_VERSION $ENV{QT_VERSION})
endif()

if(NOT QT_VERSION)
	# if QT version not specified then use any available version (5.12 or 5.15 only)
	file(GLOB FOUND_QT_VERSIONS
		LIST_DIRECTORIES true
		$ENV{HOME}/Qt/5.12.*
		$ENV{HOME}/Qt/5.15.*
	)
	if(NOT FOUND_QT_VERSIONS)
		return()
	endif()
	list(SORT FOUND_QT_VERSIONS) # prefer 5.12
	list(GET FOUND_QT_VERSIONS 0 QT_VERSION_PATH)
	get_filename_component(QT_VERSION ${QT_VERSION_PATH} NAME)	
endif()

if(DEFINED ENV{QT_MKSPEC})
	set(QT_MKSPEC $ENV{QT_MKSPEC})
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
	set(LINUX TRUE)
endif()

if(NOT QT_MKSPEC)
	if(APPLE)
		set(QT_MKSPEC clang_64)
	elseif(LINUX)
		set(QT_MKSPEC gcc_64)
	elseif(WIN32)
		set(QT_MKSPEC msvc2017_64)
		#set(QT_MKSPEC winrt_x64_msvc2017)
	endif()
endif()

set(QT_LIBRARY_HINTS
		$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
		$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
		C:/Qt
)
