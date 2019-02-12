cmake_minimum_required(VERSION 3.2)

project(QGroundControl LANGUAGES CXX)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")

# CMake build type
# Debug Release RelWithDebInfo MinSizeRel Coverage
if (NOT CMAKE_BUILD_TYPE)
	# default to release with debug symbols
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(DEFINED ENV{QT_VERSION})
	set(QT_VERSION $ENV{QT_VERSION})
endif()

if(NOT QT_VERSION)
	# try Qt 5.11.2 if none specified
	set(QT_VERSION "5.11.2")
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

# Add folder where are supportive functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "Qt spec: ${QT_MKSPEC}")

set(COMPANY "Mavlink")
set(COPYRIGHT "Copyright (c) 2018 QGroundControl. All rights reserved.")
set(IDENTIFIER "io.mavlink.qgroundcontrol")

execute_process(
	COMMAND git submodule update --init
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

execute_process(
	COMMAND git describe --always --tags
	OUTPUT_VARIABLE git_tag
	OUTPUT_STRIP_TRAILING_WHITESPACE
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

add_definitions(
	-DQGC_APPLICATION_NAME="QGroundControl"
	-DQGC_ORG_NAME="QGroundControl.org"
	-DQGC_ORG_DOMAIN="org.qgroundcontrol"
	)

message(STATUS "QGroundControl version: ${git_tag}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#=============================================================================
# ccache
#
option(CCACHE "Use ccache if available" ON)
find_program(CCACHE_PROGRAM ccache)
if (CCACHE AND CCACHE_PROGRAM AND NOT DEFINED ENV{CCACHE_DISABLE})
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

#=============================================================================
# Qt5
#
find_package(Qt5 ${QT_VERSION}
	COMPONENTS
		Bluetooth
		Concurrent
		Core
		Location
		Multimedia
		Network
		Positioning
		Quick
		QuickWidgets
		Sql
		Svg
		Test
		TextToSpeech
		Widgets
		Xml
	REQUIRED
	HINTS
		$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
		$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
		C:/Qt
)

if(NOT QT_MKSPEC MATCHES "winrt")
	find_package(Qt5 ${QT_VERSION}
		COMPONENTS
			SerialPort
		REQUIRED
		HINTS
			$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
			$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
			C:/Qt
	)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	include(CTest)
	enable_testing()
	add_definitions(-DUNITTEST_BUILD)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# clang and AppleClang
	add_compile_options(
		-Wall
		-Wextra

		-Wno-address-of-packed-member # ignore for mavlink
	)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
		add_compile_options(-fdiagnostics-color=always)
	endif()

	add_compile_options(
		-Wall
		-Wextra
	)
	
elseif (WIN32)

	add_definitions(-D_USE_MATH_DEFINES)
	add_compile_options(
		/wd4244 # warning C4244: '=': conversion from 'double' to 'float', possible loss of data
		)

endif()

# TODO: get qtquick compiler working
#qtquick_compiler_add_resources(QGC_RESOURCES ${QGC_RESOURCES}
#find_package(Qt5QuickCompiler)

include_directories(
	libs/eigen

	libs/mavlink/include/mavlink/v2.0
	libs/mavlink/include/mavlink/v2.0/ardupilotmega
	libs/mavlink/include/mavlink/v2.0/common

	libs/shapelib
)

add_subdirectory(libs)
add_subdirectory(src)

set(QGC_RESOURCES
	#HackAndroidFileDialog.qrc
	HackFileDialog.qrc
	qgcresources.qrc
	qgroundcontrol.qrc

	src/FirmwarePlugin/APM/APMResources.qrc
	src/FirmwarePlugin/PX4/PX4Resources.qrc
	src/Airmap/airmap.qrc
	src/Airmap/dummy/airmap_dummy.qrc
	)

if(BUILD_TESTING)
	list(APPEND QGC_RESOURCES
		UnitTest.qrc
	)
endif()

if(ANDROID)
	add_library(QGroundControl SHARED ${QGC_RESOURCES})
else()
	add_executable(QGroundControl ${QGC_RESOURCES})
endif()

target_link_libraries(QGroundControl PRIVATE qgc)

if(BUILD_TESTING)
        target_link_libraries(QGroundControl PRIVATE Qt5::Test)
endif()

if(NOT QT_MKSPEC MATCHES "winrt")
	target_link_libraries(QGroundControl
		PUBLIC
			Qt5::SerialPort
		)
endif()

if(LINUX)

elseif(APPLE)

	get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
	find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

	add_custom_command(TARGET QGroundControl
		POST_BUILD
		COMMAND
			${MACDEPLOYQT_EXECUTABLE} $<TARGET_FILE_DIR:QGroundControl>/../.. -appstore-compliant -qmldir=${CMAKE_SOURCE_DIR}/src
		COMMAND
			rsync -a ${CMAKE_SOURCE_DIR}/libs/lib/Frameworks $<TARGET_FILE_DIR:QGroundControl>/../../Contents/
		COMMAND
			${CMAKE_INSTALL_NAME_TOOL} -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $<TARGET_FILE:QGroundControl>
	)

	set_target_properties(QGroundControl PROPERTIES MACOSX_BUNDLE YES)

elseif(WIN32)

	if(MSVC) # Check if we are using the Visual Studio compiler
		set_target_properties(${PROJECT_NAME} PROPERTIES
			WIN32_EXECUTABLE YES
			LINK_FLAGS "/ENTRY:mainCRTStartup"
		)
	endif()

	# deploy
	include(Windeployqt)
	windeployqt(QGroundControl "QGroundControl-installer.exe")

elseif(ANDROID)
	include(AddQtAndroidApk)
	add_qt_android_apk(QGroundControl.apk QGroundControl
		PACKAGE_NAME "io.mavlink.qgroundcontrol"
		#KEYSTORE ${CMAKE_CURRENT_LIST_DIR}/mykey.keystore myalias
		#KEYSTORE_PASSWORD xxxxx
	)

elseif(LINUX)
	# TODO: investigate https://github.com/probonopd/linuxdeployqt

	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage
		COMMAND ${CMAKE_SOURCE_DIR}/deploy/create_linux_appimage.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/release/package;
		DEPENDS QGroundControl
		USES_TERMINAL
	)
	add_custom_target(appimage DEPENDS ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage)

endif()

