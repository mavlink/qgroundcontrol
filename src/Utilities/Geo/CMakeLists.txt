qt_add_library(
    QGCGeoMath STATIC
    QGCGeo.cc
    QGCGeo.h
    QGCPlanarGeometry.cc
    QGCPlanarGeometry.h
)

include(GeographicLib.cmake)

CPMAddPackage(
    NAME clipper2
    VERSION 2.0.1
    GITHUB_REPOSITORY AngusJohnson/Clipper2
    GIT_TAG Clipper2_2.0.1
    SOURCE_SUBDIR CPP
    FIND_PACKAGE_ARGUMENTS "NAMES Clipper2 CONFIG"
    OPTIONS
        "CLIPPER2_USINGZ OFF"
        "CLIPPER2_UTILS OFF"
        "CLIPPER2_EXAMPLES OFF"
        "CLIPPER2_TESTS OFF"
)

if(TARGET Clipper2::Clipper2)
    set(_qgc_clipper2_target Clipper2::Clipper2)
elseif(TARGET Clipper2)
    set(_qgc_clipper2_target Clipper2)
    qgc_disable_dependency_warnings(Clipper2)
else()
    message(FATAL_ERROR "QGC: Clipper2 (required dependency) was not resolved")
endif()

target_link_libraries(
    QGCGeoMath
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
    PRIVATE
        ${_qgc_clipper2_target}
        GeographicLib::GeographicLib
        QGCLogging
)

target_include_directories(QGCGeoMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGeoMath)

add_subdirectory(Formats)
