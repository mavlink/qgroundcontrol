find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

target_sources(QGC
    PRIVATE
        QGCGeo.cc
        QGCGeo.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Positioning
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

if(LINUX AND EXISTS "/usr/share/cmake/geographiclib")
    list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/geographiclib")
endif()

include(CPM)
# CPMAddPackage(
#     NAME geographiclib
#     VERSION 2.5
#     URL https://sourceforge.net/projects/geographiclib/files/distrib-C%2B%2B/GeographicLib-2.5.tar.gz/download
#     DOWNLOAD_ONLY
#     SOURCE_SUBDIR src
# )

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
)

if(TARGET GeographicLib::GeographicLib)
    target_link_libraries(QGC PRIVATE GeographicLib::GeographicLib)
endif()
