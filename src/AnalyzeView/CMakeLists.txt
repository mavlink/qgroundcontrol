find_package(Qt6 REQUIRED COMPONENTS Core Charts Gui Qml QmlIntegration)

qt_add_library(AnalyzeView STATIC
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

include(FetchContent)
FetchContent_Declare(ulogparser
    GIT_REPOSITORY https://github.com/PX4/ulog_cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ulogparser)

target_link_libraries(AnalyzeView
    PRIVATE
        Qt6::Charts
        Qt6::Gui
        Qt6::Qml
        ulog_cpp::ulog_cpp
        FactSystem
        QGC
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
        MAVLink
        QmlControls
)

set(MINIMUM_EXIV2_VERSION 0.28.3)

if(NOT QGC_BUILD_DEPENDENCIES)
    find_package(Exiv2 ${MINIMUM_EXIV2_VERSION} CONFIG)
    if(Exiv2_FOUND)
        message(STATUS "Found Exiv2 ${Exiv2_VERSION_STRING}")
        target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(Exiv2 IMPORTED_TARGET exiv2>=${MINIMUM_EXIV2_VERSION})
            if(Exiv2_FOUND)
                message(STATUS "Found Exiv2 ${Exiv2_VERSION}")
                target_link_libraries(AnalyzeView PRIVATE PkgConfig::Exiv2)
            endif()
        endif()
    endif()
endif()

if(NOT Exiv2_FOUND)
    message(STATUS "Building Exiv2")
    include(FetchContent)
    FetchContent_Declare(EXIV2
        GIT_REPOSITORY https://github.com/Exiv2/exiv2.git
        GIT_TAG v0.28.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    set(EXIV2_ENABLE_XMP OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_EXTERNAL_XMP OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_PNG OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_NLS OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_LENSDATA OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_DYNAMIC_RUNTIME ON CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_WEBREADY OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_CURL OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_BMFF OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_BROTLI OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_VIDEO OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_INIH OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_ENABLE_FILESYSTEM_ACCESS ON CACHE INTERNAL "" FORCE)
    set(EXIV2_BUILD_SAMPLES OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_BUILD_EXIV2_COMMAND OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_BUILD_UNIT_TESTS OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_BUILD_FUZZ_TESTS OFF CACHE INTERNAL "" FORCE)
    set(EXIV2_BUILD_DOC OFF CACHE INTERNAL "" FORCE)
    set(BUILD_WITH_CCACHE ON CACHE INTERNAL "" FORCE)
    FetchContent_MakeAvailable(EXIV2)

    target_link_libraries(AnalyzeView PRIVATE exiv2lib)
    target_include_directories(AnalyzeView
        PRIVATE
            ${CMAKE_BINARY_DIR}
            ${exiv2_SOURCE_DIR}/include
            ${exiv2_SOURCE_DIR}/include/exiv2
    )
endif()

include(FetchContent)
FetchContent_Declare(easyexif
    GIT_REPOSITORY https://github.com/mayanklahiri/easyexif.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(easyexif)

target_sources(AnalyzeView
    PRIVATE
        ${easyexif_SOURCE_DIR}/exif.cpp
        ${easyexif_SOURCE_DIR}/exif.h
)

target_include_directories(AnalyzeView
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${easyexif_SOURCE_DIR}
)

# qt_add_qml_module(AnalyzeView
#     URI QGroundControl.AnalyzeView
#     VERSION 1.0
#     QML_FILES
#       AnalyzePage.qml
#       AnalyzeView.qml
#       GeoTagPage.qml
#       LogDownloadPage.qml
#       MAVLinkConsolePage.qml
#       MAVLinkInspectorPage.qml
#       VibrationPage.qml
#     RESOURCES
#       FloatingWindow.svg
#       GeoTagIcon.svg
#       LogDownloadIcon.svg
#       MAVLinkConsoleIcon.svg
#       MAVLinkInspector.svg
#       VibrationPageIcon.png
#     OUTPUT_TARGETS AnalyzeView_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtQuick.Dialogs
#       QtQuick.Layouts
#       QtQuick.Window
#       QtCharts
#       Qt.labs.qmlmodels
#       QGroundControl
#       QGroundControl.Palette
#       QGroundControl.Controls
#       QGroundControl.Controllers
#       QGroundControl.FactSystem
#       QGroundControl.FactControls
#       QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )
