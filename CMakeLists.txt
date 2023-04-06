cmake_minimum_required(VERSION 3.10)

project(QGroundControl LANGUAGES C CXX)

include(GNUInstallDirs)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FeatureSummary)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-Wall -Wextra)
endif()

# CMake build type
# Debug Release RelWithDebInfo MinSizeRel Coverage
if (NOT CMAKE_BUILD_TYPE)
	# default to release with debug symbols
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Add folder where are supportive functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Configure Qt5 to get necessary variables
include(Qt5QGCConfiguration)
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "Qt spec: ${QT_MKSPEC}")

set(COMPANY "Mavlink")
set(COPYRIGHT "Copyright (c) 2018 QGroundControl. All rights reserved.")
set(IDENTIFIER "io.mavlink.qgroundcontrol")

add_definitions(
	-DQGC_APPLICATION_NAME="QGroundControl"
	-DQGC_ORG_NAME="QGroundControl.org"
	-DQGC_ORG_DOMAIN="org.qgroundcontrol"
	)

include(Git)
message(STATUS "QGroundControl version: ${APP_VERSION_STR}")

#=============================================================================
# ccache
#
option(CCACHE "Use ccache if available" ON)
find_program(CCACHE_PROGRAM ccache)
if (CCACHE AND CCACHE_PROGRAM AND NOT DEFINED ENV{CCACHE_DISABLE})
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

#=============================================================================
# Compile QML
#
option(COMPILE_QML "Pre-compile QML files using the Qt Quick compiler." FALSE)
add_feature_info(COMPILE_QML COMPILE_QML "Pre-compile QML files using the Qt Quick compiler.")
if(COMPILE_QML)
    find_package(Qt5QuickCompiler)

    set_package_properties(Qt5QuickCompiler PROPERTIES
        DESCRIPTION "Pre-compile QML files using the Qt Quick compiler."
        TYPE OPTIONAL
    )
endif()

#=============================================================================
# Debug QML
#
option(DEBUG_QML "Build QGroundControl with QML debugging/profiling support." FALSE)
add_feature_info(DEBUG_QML DEBUG_QML "Build QGroundControl with QML debugging/profiling support.")
if(DEBUG_QML)
    message(STATUS "To enable the QML debugger/profiler, run with: '-qmljsdebugger=port:1234'")
    add_definitions(-DQMLJSDEBUGGER)
    add_definitions(-DQT_DECLARATIVE_DEBUG)
    add_definitions(-DQT_QML_DEBUG)
endif()

#=============================================================================
# GStreamer
#
find_package(PkgConfig)

set(GST_DEPENDENCIES
	gstreamer-1.0>=1.14
	gstreamer-video-1.0>=1.14
	gstreamer-gl-1.0>=1.14
	)

if (MSVC)
	pkg_check_modules(GST
		${GST_DEPENDENCIES}
		)
else()
	pkg_check_modules(GST
		${GST_DEPENDENCIES}
		egl
		)
endif()

if (GST_FOUND)
    add_definitions(
        -DQGC_GST_STREAMING
    )

    option(QGC_GST_MICROHARD_ENABLED "Enable microhard" OFF)
    option(QGC_GST_TAISYNC_ENABLED "Enable taisyng" OFF)
else()
	if (QGC_GST_MICROHARD_ENABLED OR QGC_GST_TAISYNC_ENABLED)
		message(FATAL_ERROR "You tried to enable Microhard or Taisync but gstreamer is not found. Make sure to set PKG_CONFIG_EXECUTABLE and/or PKG_CONFIG_PATH properly.")
	endif()
endif()

#=============================================================================
# Qt5
#
find_package(Qt5 ${QT_VERSION}
	COMPONENTS
		Bluetooth
		Charts
		Concurrent
		Core
		Location
		Multimedia
		Network
		Positioning
		Quick
		QuickControls2
		QuickWidgets
		OpenGL
		Sql
		Svg
		Test
		TextToSpeech
		Widgets
		Xml
	REQUIRED
	HINTS
		${QT_LIBRARY_HINTS}
)

if(NOT QT_MKSPEC MATCHES "winrt")
	find_package(Qt5 ${QT_VERSION}
		COMPONENTS
			SerialPort
		REQUIRED
		HINTS
			${QT_LIBRARY_HINTS}
	)
endif()

# Sets the default flags for compilation and linking.
include(CompileOptions)

include_directories(
	libs/eigen

	libs/libevents

	libs/mavlink/include/mavlink/v2.0
	libs/mavlink/include/mavlink/v2.0/ardupilotmega
	libs/mavlink/include/mavlink/v2.0/common

	libs/shapelib
)

add_subdirectory(libs)
add_subdirectory(src)

set(QGC_RESOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/qgcimages.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/qgcresources.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/qgroundcontrol.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/src/Airmap/airmap.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/src/Airmap/dummy/airmap_dummy.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/APM/APMResources.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/PX4/PX4Resources.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiverApp/qml.qrc
	)

if (WIN32)
	# append application icon resource for Windows
	set(QGC_RESOURCES
		${QGC_RESOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/windows/QGroundControl.rc)
endif()

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

# Files/directories to install
install(
    TARGETS QGroundControl
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/qgroundcontrol
)
install(
    FILES ${CMAKE_SOURCE_DIR}/deploy/org.mavlink.qgroundcontrol.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
    FILES ${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
    RENAME org.mavlink.qgroundcontrol.png
)
configure_file(
    ${CMAKE_SOURCE_DIR}/deploy/org.mavlink.qgroundcontrol.metainfo.xml.in
    ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
    @ONLY
)
install(
    FILES ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
)

if(BUILD_TESTING)
        target_link_libraries(QGroundControl PRIVATE Qt5::Test)
endif()

if(NOT QT_MKSPEC MATCHES "winrt")
	target_link_libraries(QGroundControl
		PUBLIC
			Qt5::SerialPort
		)
endif()

include(QGCDeploy)
