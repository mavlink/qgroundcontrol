# ============================================================================
# Geo Subsystem
# Geographic data handling (GeoJSON, KML, Shapefiles)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GeoJsonHelper.cc
        GeoJsonHelper.h
        KMLDomDocument.cc
        KMLDomDocument.h
        KMLHelper.cc
        KMLHelper.h
        QGCGeo.cc
        QGCGeo.h
        ShapeFileHelper.cc
        ShapeFileHelper.h
        SHPFileHelper.cc
        SHPFileHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# GeographicLib Integration
# ============================================================================

CPMAddPackage(
    NAME geographiclib
    VERSION 2.5
    GITHUB_REPOSITORY geographiclib/geographiclib
    GIT_TAG r2.5
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

# Suppress MSVC warnings in GeographicLib
target_compile_options(GeographicLib_STATIC PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd9025>)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeographicLib::GeographicLib)

# ============================================================================
# Shapelib Integration
# ============================================================================

CPMAddPackage(
    NAME Shapelib
    VERSION 1.6.1
    GITHUB_REPOSITORY OSGeo/shapelib
    OPTIONS
        "BUILD_SHAPELIB_CONTRIB OFF"
        "BUILD_APPS OFF"
        "BUILD_TESTING OFF"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shp)
