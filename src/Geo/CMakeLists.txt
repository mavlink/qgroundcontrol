find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(Geo STATIC
    QGCGeo.cc
    QGCGeo.h
)

target_link_libraries(Geo
    PRIVATE
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Positioning
)

target_include_directories(Geo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

set(MINIMUM_GEOGRAPHICLIB_VERSION 2.3)
set(TARGET_GEOGRAPHICLIB_VERSION 2.5)

if(NOT QGC_BUILD_DEPENDENCIES)
    if(LINUX)
        list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/geographiclib")
    endif()
    find_package(GeographicLib ${MINIMUM_GEOGRAPHICLIB_VERSION} COMPONENTS STATIC)
    if(TARGET GeographicLib::GeographicLib)
        target_link_libraries(Geo PRIVATE GeographicLib::GeographicLib)
        return()
    elseif(GeographicLib_FOUND)
        target_link_libraries(Geo PRIVATE ${GeographicLib_LIBRARIES})
        target_include_directories(Geo PRIVATE ${GeographicLib_INCLUDE_DIRS})
        target_link_directories(Geo PRIVATE ${GeographicLib_LIBRARY_DIRS})
        return()
    endif()
endif()

message(STATUS "Building GeographicLib")

include(CPM)
CPMAddPackage(
    NAME geographiclib
    VERSION ${TARGET_GEOGRAPHICLIB_VERSION}
    GITHUB_REPOSITORY geographiclib/geographiclib
    GIT_TAG "r${TARGET_GEOGRAPHICLIB_VERSION}"
    GIT_SHALLOW TRUE
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
    target_link_libraries(Geo PRIVATE GeographicLib::GeographicLib)
endif()
