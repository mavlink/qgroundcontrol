if(DEFINED ENV{QT_VERSION})
	set(QT_VERSION $ENV{QT_VERSION})
endif()

if(NOT QT_VERSION)
	# try Qt 5.12.0 if none specified, last LTS.
	set(QT_VERSION "5.12.5")
endif()

if(DEFINED ENV{QT_MKSPEC})
	set(QT_MKSPEC $ENV{QT_MKSPEC})
endif()

if(UNIX AND NOT APPLE)
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
