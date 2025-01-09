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

target_link_libraries(AnalyzeView
    PRIVATE
        Qt6::Charts
        Qt6::Gui
        Qt6::Qml
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

target_include_directories(AnalyzeView PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(AnalyzeView
    PRIVATE
        <QtCharts/QAbstractSeries>
)

# qt_add_qml_module(AnalyzeView
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

message(STATUS "Building ULogParser")
FetchContent_Declare(ulogparser
    GIT_REPOSITORY https://github.com/PX4/ulog_cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ulogparser)
if(TARGET ulog_cpp::ulog_cpp)
    target_link_libraries(AnalyzeView PRIVATE ulog_cpp::ulog_cpp)
endif()

#===========================================================================#

set(MINIMUM_EXIV2_VERSION 0.28.0)
set(TARGET_EXIV2_VERSION 0.28.3)

set(MINIMUM_EXPAT_VERSION 2.2.6)
set(TARGET_EXPAT_VERSION 2.6.4)

# if(QGC_DOWNLOAD_DEPENDENCIES)
#     set(EXIV2_URL)
#     if(MSVC)
#         set(EXIV2_URL "https://github.com/Exiv2/exiv2/releases/download/v${TARGET_EXIV2_VERSION}/exiv2-${TARGET_EXIV2_VERSION}-2019msvc64.zip")
#     elseif(LINUX)
#         set(EXIV2_URL "https://github.com/Exiv2/exiv2/releases/download/v${TARGET_EXIV2_VERSION}/exiv2-${TARGET_EXIV2_VERSION}-Linux64.tar.gz")
#     elseif(MACOS)
#         set(EXIV2_URL "https://github.com/Exiv2/exiv2/releases/download/v${TARGET_EXIV2_VERSION}/exiv2-${TARGET_EXIV2_VERSION}-Darwin.tar.gz")
#     endif()

#     if(EXIV2_URL)
#         FetchContent_Declare(exiv2
#            DOWNLOAD_EXTRACT_TIMESTAMP true
#            URL ${EXIV2_URL}
#         )
#         FetchContent_MakeAvailable(exiv2)

#         # exiv2_SOURCE_DIR
#         # target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
#         return()
#     endif()
# endif()

if(NOT QGC_BUILD_DEPENDENCIES)
    find_package(exiv2 ${MINIMUM_EXIV2_VERSION})
    if(TARGET Exiv2::exiv2lib)
        target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
        return()
    endif()
endif()

if(NOT QGC_BUILD_DEPENDENCIES)
    find_package(EXPAT ${MINIMUM_EXPAT_VERSION})
endif()
if(NOT TARGET EXPAT::EXPAT)
    message(STATUS "Building Expat")
    FetchContent_Declare(expat
        GIT_REPOSITORY https://github.com/libexpat/libexpat.git
        GIT_TAG R_2_6_4
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR expat
    )
    set(EXPAT_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_BUILD_FUZZERS OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_BUILD_PKGCONFIG OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
    set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    FetchContent_MakeAvailable(expat)
endif()

message(STATUS "Building Exiv2")
FetchContent_Declare(exiv2
    GIT_REPOSITORY https://github.com/Exiv2/exiv2.git
    GIT_TAG "v${TARGET_EXIV2_VERSION}"
    GIT_SHALLOW TRUE
)
if(TARGET EXPAT::EXPAT)
    set(EXIV2_ENABLE_XMP ON CACHE INTERNAL "" FORCE)
else()
    set(EXIV2_ENABLE_XMP OFF CACHE INTERNAL "" FORCE)
endif()
set(EXIV2_ENABLE_EXTERNAL_XMP OFF CACHE INTERNAL "" FORCE)
set(EXIV2_ENABLE_PNG OFF CACHE INTERNAL "" FORCE)
set(EXIV2_ENABLE_NLS OFF CACHE INTERNAL "" FORCE)
set(EXIV2_ENABLE_LENSDATA OFF CACHE INTERNAL "" FORCE)
set(EXIV2_ENABLE_DYNAMIC_RUNTIME OFF CACHE INTERNAL "" FORCE)
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
FetchContent_MakeAvailable(exiv2)

if(TARGET Exiv2::exiv2lib)
    target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
    target_include_directories(AnalyzeView PRIVATE ${CMAKE_BINARY_DIR})
    return()
endif()
