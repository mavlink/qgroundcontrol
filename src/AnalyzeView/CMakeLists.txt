set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ExifParser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ExifParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagWorker.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagWorker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogDownloadController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LogDownloadController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogEntry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LogEntry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkChartController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkChartController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkConsoleController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkConsoleController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkInspectorController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkInspectorController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkMessage.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkMessage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkMessageField.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkMessageField.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkSystem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4LogParser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4LogParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ULogParser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ULogParser.h
    CACHE INTERNAL "" FORCE
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Qt6::Charts
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG main
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    ulog_cpp::ulog_cpp
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

set(EXPAT_EXTRA_OPTIONS)
if(WIN32)
    list(APPEND EXPAT_EXTRA_OPTIONS "EXPAT_MSVC_STATIC_CRT ON")
endif()

CPMAddPackage(
    NAME EXPAT
    VERSION 2.7.0
    GITHUB_REPOSITORY libexpat/libexpat
    GIT_TAG "R_2_7_0"
    SOURCE_SUBDIR expat
    OPTIONS
        "EXPAT_BUILD_DOCS OFF"
        "EXPAT_BUILD_EXAMPLES OFF"
        "EXPAT_BUILD_FUZZERS OFF"
        "EXPAT_BUILD_PKGCONFIG OFF"
        "EXPAT_BUILD_TESTS OFF"
        "EXPAT_BUILD_TOOLS OFF"
        "EXPAT_ENABLE_INSTALL OFF"
        "EXPAT_SHARED_LIBS OFF"
        "${EXPAT_EXTRA_OPTIONS}"
)

set(EXPAT_USE_STATIC_LIBS ON)
find_package(EXPAT MODULE)
if(NOT TARGET EXPAT::EXPAT)
    find_package(expat CONFIG)
    if(TARGET expat::expat)
        qt_add_library(EXPAT::EXPAT INTERFACE IMPORTED)
        target_link_libraries(EXPAT::EXPAT INTERFACE expat::expat)
    endif()
endif()

if(TARGET EXPAT::EXPAT)
    set(_EXIV2_ENABLE_XMP ON)
else()
    set(_EXIV2_ENABLE_XMP OFF)
endif()

#===========================================================================#

CPMAddPackage(
    NAME exiv2
    VERSION 0.28.5
    GITHUB_REPOSITORY Exiv2/exiv2
    OPTIONS
        "EXIV2_ENABLE_XMP ${_EXIV2_ENABLE_XMP}"
        "EXIV2_ENABLE_EXTERNAL_XMP OFF"
        "EXIV2_ENABLE_PNG OFF"
        "EXIV2_ENABLE_NLS OFF"
        "EXIV2_ENABLE_LENSDATA OFF"
        "EXIV2_ENABLE_DYNAMIC_RUNTIME OFF"
        "EXIV2_ENABLE_WEBREADY OFF"
        "EXIV2_ENABLE_CURL OFF"
        "EXIV2_ENABLE_BMFF OFF"
        "EXIV2_ENABLE_BROTLI OFF"
        "EXIV2_ENABLE_VIDEO OFF"
        "EXIV2_ENABLE_INIH OFF"
        "EXIV2_ENABLE_FILESYSTEM_ACCESS OFF"
        "EXIV2_BUILD_SAMPLES OFF"
        "EXIV2_BUILD_EXIV2_COMMAND OFF"
        "EXIV2_BUILD_UNIT_TESTS OFF"
        "EXIV2_BUILD_FUZZ_TESTS OFF"
        "EXIV2_BUILD_DOC OFF"
        "BUILD_WITH_CCACHE ON"
)

if(_EXIV2_ENABLE_XMP AND TARGET exiv2-xmp)
    target_link_libraries(exiv2-xmp PRIVATE EXPAT::EXPAT)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(exiv2-xmp PRIVATE -Wno-unused-command-line-argument -Wno-unused-but-set-variable -Wno-format)
    endif()
endif()

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Exiv2::exiv2lib
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_BINARY_DIR}
    CACHE INTERNAL "" FORCE
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(exiv2lib PRIVATE -Wno-unused-command-line-argument -Wno-unused-variable)
    target_compile_options(exiv2lib_int PRIVATE -Wno-unused-command-line-argument -Wno-unused-parameter)
endif()

#===========================================================================#

# qt_add_qml_module(QGC
#     URI QGroundControl.AnalyzeView
#     VERSION 1.0
#     QML_FILES
#         AnalyzePage.qml
#         AnalyzeView.qml
#         GeoTagPage.qml
#         LogDownloadPage.qml
#         MAVLinkConsolePage.qml
#         MAVLinkInspectorPage.qml
#         VibrationPage.qml
#     RESOURCES
#         FloatingWindow.svg
#         GeoTagIcon.svg
#         LogDownloadIcon.svg
#         MAVLinkConsoleIcon.svg
#         MAVLinkInspector.svg
#         VibrationPageIcon.png
#     OUTPUT_TARGETS AnalyzeView_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QGroundControl
#         QGroundControl.Controllers
#         QGroundControl.Controls
#         QGroundControl.FactControls
#         QGroundControl.FactSystem
#         QGroundControl.Palette
#         QGroundControl.ScreenTools
#         Qt.labs.qmlmodels
#         QtCharts
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts
#         QtQuick.Window
#     DEPENDENCIES
#         QtCore
# )
