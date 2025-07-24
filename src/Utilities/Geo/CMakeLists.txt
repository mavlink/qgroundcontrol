target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCGeo.cc
        QGCGeo.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
)

target_compile_options(GeographicLib_STATIC PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd9025>)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeographicLib::GeographicLib)
