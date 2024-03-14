cmake_minimum_required(VERSION 3.21.1 FATAL_ERROR)

project(VideoReceiverApp VERSION 1.0 LANGUAGES C CXX)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickWidgets Multimedia)

qt_standard_project_setup(REQUIRES 6.6.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/staging)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "Install path for QML" FORCE)
set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml" CACHE STRING "Extra QML Import Paths" FORCE)

qt_policy(
    SET QTP0001 NEW
    SET QTP0002 NEW
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Release build")
    set(QT_QML_DEBUG OFF)
else()
    message("Debug build")
    set(QT_QML_DEBUG ON)
endif()

qt_add_executable(${PROJECT_NAME} main.cpp)

if(ANDROID)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            QT_ANDROID_ABIS ${ANDROID_ABI}
            #QT_ANDROID_EXTRA_LIBS "${ANDROID_EXTRA_LIBS_APP}"
            QT_ANDROID_MIN_SDK_VERSION 30
            QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
            QT_ANDROID_SDK_BUILD_TOOLS_REVISION 34.0.0
            QT_ANDROID_TARGET_SDK_VERSION 33
            QT_ANDROID_VERSION_NAME '1.0'
            QT_ANDROID_VERSION_CODE 1.0
            QT_QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_sources(${PROJECT_NAME} PRIVATE android/init.cpp)
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

set(QGC_ROOT ${CMAKE_SOURCE_DIR}/..)

add_subdirectory(${QGC_ROOT}/libs/qmlglsink qmlglsink.build)
if(NOT GST_FOUND)
    message(FATAL_ERROR "No GStreamer!")
endif()
include_directories(VideoReceiver
    PRIVATE
        ${QGC_ROOT}/src
        ${QGC_ROOT}/src/Settings
)
add_subdirectory(${QGC_ROOT}/src/VideoReceiver VideoReceiver.build)

target_include_directories(VideoReceiver
    PRIVATE
        ${GST_INCLUDE_DIRS}
        ${QGC_ROOT}/src
        ${QGC_ROOT}/src/Settings
)

target_sources(${PROJECT_NAME} PRIVATE ${QGC_ROOT}/src/QGCLoggingCategory.cc)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        VideoReceiver
        Qt6::Core
        Qt6::Multimedia
        Qt6::OpenGL
        Qt6::Quick
        Qt6::QuickWidgets
)

qt_add_qml_module(${PROJECT_NAME}
    URI videoreceiverapp
    VERSION 1.0
    QML_FILES
        main.qml
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
        QtCore
        QtQuick
        QtQuick.Layouts
        QtQuick.Window
        QtMultimedia
)

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(NOT ANDROID)
    qt_generate_deploy_qml_app_script(TARGET ${PROJECT_NAME} OUTPUT_SCRIPT deploy_script)
    install(SCRIPT ${deploy_script})
endif()
