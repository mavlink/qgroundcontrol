set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCGeo.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCGeo.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

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
    # EXCLUDE_FROM_ALL
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    GeographicLib::GeographicLib
    CACHE INTERNAL "" FORCE
)
