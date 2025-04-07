target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCGeo.cc
        QGCGeo.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

if(LINUX AND EXISTS "/usr/share/cmake/geographiclib")
    list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/geographiclib")
endif()

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

if(TARGET GeographicLib::GeographicLib)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeographicLib::GeographicLib)
endif()
