if(UNIX AND NOT APPLE AND NOT ANDROID)
	set(LINUX TRUE)
endif()

if(APPLE AND NOT IOS)
	set(MACOS TRUE)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	include(CTest)
	enable_testing()
	if(QGC_BUILD_TESTING)
        message("Building tests")
		add_compile_definitions(UNITTEST_BUILD) # TODO: QGC_UNITTEST_BUILD
	else()
		# will prevent the definition of QT_DEBUG, which enables code that uses MockLink
		add_compile_definitions(QT_NO_DEBUG)
	endif()
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_compile_definitions(QGC_INSTALL_RELEASE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# clang and AppleClang
	add_compile_options(
		-Wall
		-Wextra
		-Wno-address-of-packed-member # ignore for mavlink
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
		add_compile_options(-fdiagnostics-color=always)
	endif()

	add_compile_options(
		-Wall
		-Wextra
	)
elseif(WIN32)
	add_compile_definitions(_USE_MATH_DEFINES)
	add_compile_options(
		/wd4244 # warning C4244: '=': conversion from 'double' to 'float', possible loss of data
    )
endif()

add_compile_definitions(
    QT_DISABLE_DEPRECATED_BEFORE=0x060600
    QT_DEBUG_FIND_PACKAGE=ON
)

if(ANDROID OR IOS)
	set(MOBILE TRUE)
	add_compile_definitions(__mobile__)
endif()

if(ANDROID)
	add_compile_definitions(__android__)
elseif(IOS)
	add_compile_definitions(__ios__)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/ardupilotmega)
	add_compile_definitions(NO_ARDUPILOT_DIALECT) # TODO: Make this QGC_NO_ARDUPILOT_DIALECT
endif()

# option(QGC_CUSTOM_BUILD "Enable Custom Build" OFF)
# option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable Mavlink Inspector" OFF) # This removes QtCharts which is GPL licensed
