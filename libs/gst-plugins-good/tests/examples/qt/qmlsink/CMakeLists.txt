cmake_minimum_required(VERSION 3.3)

include(FindPkgConfig)
if(!${PKG_CONFIG_FOUND})
  MESSAGE(FATAL_ERROR "pkg-config required. Please install it")
  return ("Error - pkg-config not found")
endif()

pkg_check_modules(GSTREAMER gstreamer-1.0>=1.6 gstreamer-video-1.0>=1.6 REQUIRED)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

set (SRC_LIST main.cpp)
qt5_add_resources(RESOURCES qmlsink.qrc)
link_directories(${GSTREAMER_LIBRARY_DIRS})
include_directories (${GSTREAMER_INCLUDE_DIRS})
add_executable(qml-example ${SRC_LIST} ${RESOURCES})
target_link_libraries (qml-example ${GSTREAMER_LIBRARIES})
qt5_use_modules(qml-example Core Widgets Qml Quick)

