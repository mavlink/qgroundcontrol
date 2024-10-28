find_package(Qt6 REQUIRED COMPONENTS Core Charts Gui Qml QmlIntegration)

qt_add_library(AnalyzeView STATIC
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

#===========================================================================#

include(FetchContent)
FetchContent_Declare(ulogparser
    GIT_REPOSITORY https://github.com/PX4/ulog_cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ulogparser)
if(TARGET ulog_cpp::ulog_cpp)
    target_link_libraries(AnalyzeView PRIVATE ulog_cpp::ulog_cpp)
    target_sources(AnalyzeView PRIVATE ULogParser.cc ULogParser.h)
endif()

#===========================================================================#

set(MINIMUM_EXIV2_VERSION 0.28.2)

if(NOT QGC_BUILD_DEPENDENCIES)
    find_package(LibExiv2 ${MINIMUM_EXIV2_VERSION})
    if(LibExiv2_FOUND AND TARGET LibExiv2::LibExiv2)
        target_link_libraries(AnalyzeView PRIVATE LibExiv2::LibExiv2)
    else()
        find_package(exiv2 ${MINIMUM_EXIV2_VERSION} CONFIG NAMES exiv2)
        if(exiv2_FOUND)
            target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
        endif()
    endif()
endif()

if(NOT exiv2_FOUND AND NOT LibExiv2_FOUND)
    message(STATUS "Building Exiv2")

    include(FetchContent)

    # TODO: XMP Compatibility
    # FetchContent_Declare(EXPAT
    #     GIT_REPOSITORY https://github.com/libexpat/libexpat.git
    #     GIT_TAG R_2_6_3
    #     GIT_SHALLOW TRUE
    #     GIT_PROGRESS TRUE
    #     SOURCE_SUBDIR expat
    # )
    # set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    # set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
    # set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
    # FetchContent_MakeAvailable(EXPAT)

    # find_package(Iconv)
    # if(ICONV_FOUND)
    #     target_link_libraries(AnalyzeView PRIVATE Iconv::Iconv)
    # endif()

    FetchContent_Declare(EXIV2
        GIT_REPOSITORY https://github.com/Exiv2/exiv2.git
        GIT_TAG v0.28.3
        GIT_SHALLOW TRUE
    )
    set(EXIV2_ENABLE_XMP OFF CACHE INTERNAL "" FORCE)
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
    FetchContent_MakeAvailable(EXIV2)

    target_link_libraries(AnalyzeView PRIVATE Exiv2::exiv2lib)
    target_include_directories(AnalyzeView
        PRIVATE
            ${CMAKE_BINARY_DIR}
            ${exiv2_SOURCE_DIR}/include
            ${exiv2_SOURCE_DIR}/include/exiv2
    )
endif()

target_sources(AnalyzeView PRIVATE ExifParser.cc ExifParser.h)

#===========================================================================#

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
