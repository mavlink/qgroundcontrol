find_package(Qt6 REQUIRED COMPONENTS Core Charts Gui Qml QmlIntegration)

target_sources(QGC
    PRIVATE
        ExifParser.cc
        ExifParser.h
        GeoTagController.cc
        GeoTagController.h
        GeoTagWorker.cc
        GeoTagWorker.h
        LogDownloadController.cc
        LogDownloadController.h
        LogEntry.cc
        LogEntry.h
        MAVLinkChartController.cc
        MAVLinkChartController.h
        MAVLinkConsoleController.cc
        MAVLinkConsoleController.h
        MAVLinkInspectorController.cc
        MAVLinkInspectorController.h
        MAVLinkMessage.cc
        MAVLinkMessage.h
        MAVLinkMessageField.cc
        MAVLinkMessageField.h
        MAVLinkSystem.cc
        MAVLinkSystem.h
        PX4LogParser.cc
        PX4LogParser.h
        ULogParser.cc
        ULogParser.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Charts
        Qt6::Gui
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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

#===========================================================================#

include(CPM)
CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG main
)

if(TARGET ulog_cpp::ulog_cpp)
    target_link_libraries(QGC PRIVATE ulog_cpp::ulog_cpp)
endif()

CPMAddPackage(
    NAME EXPAT
    VERSION 2.6.4
    GITHUB_REPOSITORY libexpat/libexpat
    GIT_TAG "R_2_6_4"
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
)

set(_EXIV2_ENABLE_XMP OFF)
if(EXPAT_ADDED)
    find_package(expat CONFIG QUIET)
    if(NOT TARGET expat::expat)
        find_package(EXPAT MODULE)
        if(EXPAT_FOUND)
            if(NOT TARGET expat::expat)
                add_library(expat::expat INTERFACE IMPORTED)
                target_link_libraries(expat::expat INTERFACE EXPAT::EXPAT)
            endif()
            set(_EXIV2_ENABLE_XMP ON)
        endif()
    endif()
endif()

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
        "BUILD_WITH_CCACHE OFF"
        "BUILD_SHARED_LIBS OFF"
)

if(TARGET Exiv2::exiv2lib)
    target_link_libraries(QGC PRIVATE Exiv2::exiv2lib)
    target_include_directories(QGC PRIVATE ${CMAKE_BINARY_DIR})
endif()
