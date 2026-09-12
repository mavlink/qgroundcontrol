qt_add_library(QGCGeoMath STATIC
    QGCGeo.cc
    QGCGeo.h
    QGCPolygonClipper.cc
    QGCPolygonClipper.h
)

CPMAddPackage(
    NAME geographiclib
    VERSION 2.7
    GITHUB_REPOSITORY geographiclib/geographiclib
    GIT_TAG r2.7
    # System package installs its config as `GeographicLib`, not `geographiclib`.
    FIND_PACKAGE_ARGUMENTS "NAMES GeographicLib"
    OPTIONS
        "BUILD_BOTH_LIBS OFF"
        "BUILD_DOCUMENTATION OFF"
        "BUILD_MANPAGES OFF"
        "PACKAGE_DEBUG_LIBS OFF"
        "APPLE_MULTIPLE_ARCHITECTURES OFF"
        "INCDIR OFF"
        "BINDIR OFF"
        "SBINDIR OFF"
        "LIBDIR ${CMAKE_INSTALL_LIBDIR}"
        "DLLDIR ${CMAKE_INSTALL_BINDIR}"
        "MANDIR OFF"
        "CMAKEDIR OFF"
        "PKGDIR OFF"
        "DOCDIR OFF"
        "EXAMPLEDIR OFF"
    PATCHES geographiclib.patch
)

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

# geographiclib_ADDED is set only when CPM downloads; geographiclib_FOUND when
# resolved via find_package (QGC_USE_SYSTEM_LIBS). Either is success.
if(NOT geographiclib_ADDED AND NOT geographiclib_FOUND)
    message(FATAL_ERROR "QGC: geographiclib (required dependency) was not resolved")
endif()

# Only the CPM-built static target needs warning suppression; the system import
# target (GeographicLib::GeographicLib) carries no QGC-controlled warning flags.
if(TARGET GeographicLib_STATIC)
    qgc_disable_dependency_warnings(GeographicLib_STATIC)
endif()

if(TARGET Clipper2::Clipper2)
    set(_qgc_clipper2_target Clipper2::Clipper2)
elseif(TARGET Clipper2)
    set(_qgc_clipper2_target Clipper2)
    qgc_disable_dependency_warnings(Clipper2)
else()
    message(FATAL_ERROR "QGC: Clipper2 (required dependency) was not resolved")
endif()

target_link_libraries(QGCGeoMath
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
