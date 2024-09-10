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

set(MINIMUM_GEOGRAPHICLIB_VERSION 2.3)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GeographicLib ${MINIMUM_GEOGRAPHICLIB_VERSION} COMPONENTS STATIC)
    if(GeographicLib_FOUND)
        message(STATUS "Found GeographicLib")
        target_link_libraries(Geo PRIVATE ${GeographicLib_LIBRARIES})
        target_include_directories(Geo PRIVATE ${GeographicLib_INCLUDE_DIRS})
        target_link_directories(Geo PRIVATE ${GeographicLib_LIBRARY_DIRS})
        cmake_print_variables(GeographicLib_LIBRARIES GeographicLib_INCLUDE_DIRS GeographicLib_LIBRARY_DIRS)
        return()
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(GeographicLib IMPORTED_TARGET GeographicLib>=${MINIMUM_GEOGRAPHICLIB_VERSION})
            if(GeographicLib_FOUND)
                message(STATUS "Found GeographicLib ${GeographicLib_VERSION}")
                target_link_libraries(Geo PRIVATE PkgConfig::GeographicLib)
                return()
            endif()
        endif()
    endif()
endif()

message(STATUS "Building GeographicLib")
include(FetchContent)
FetchContent_Declare(GeographicLib
    GIT_REPOSITORY https://github.com/geographiclib/geographiclib.git
    GIT_TAG r2.4
    GIT_SHALLOW TRUE
)
set(BUILD_BOTH_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE INTERNAL "" FORCE)
set(BUILD_MANPAGES OFF CACHE INTERNAL "" FORCE)
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
target_link_libraries(Geo PRIVATE GeographicLib::GeographicLib)
