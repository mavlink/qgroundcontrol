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
FetchContent_Declare(GeographicLib
    GIT_REPOSITORY https://github.com/geographiclib/geographiclib.git
    GIT_TAG "r${TARGET_GEOGRAPHICLIB_VERSION}"
    GIT_SHALLOW TRUE
)
set(BUILD_BOTH_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE INTERNAL "" FORCE)
set(BUILD_MANPAGES OFF CACHE INTERNAL "" FORCE)
set(PACKAGE_DEBUG_LIBS OFF CACHE INTERNAL "" FORCE)
set(APPLE_MULTIPLE_ARCHITECTURES OFF CACHE INTERNAL "" FORCE)
set(INCDIR "" CACHE INTERNAL "" FORCE)
set(BINDIR "" CACHE INTERNAL "" FORCE)
set(SBINDIR "" CACHE INTERNAL "" FORCE)
set(LIBDIR "${CMAKE_INSTALL_LIBDIR}" CACHE STRING "" FORCE)
set(DLLDIR "${CMAKE_INSTALL_BINDIR}" CACHE INTERNAL "" FORCE)
set(MANDIR "" CACHE INTERNAL "" FORCE)
set(CMAKEDIR "" CACHE INTERNAL "" FORCE)
set(PKGDIR "" CACHE INTERNAL "" FORCE)
set(DOCDIR "" CACHE INTERNAL "" FORCE)
set(EXAMPLEDIR "" CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(GeographicLib)

if(TARGET GeographicLib::GeographicLib)
    target_link_libraries(Geo PRIVATE GeographicLib::GeographicLib)
    return()
endif()
