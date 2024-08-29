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

if(NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(geographiclib IMPORTED_TARGET geographiclib>=2.3)
        if(geographiclib_FOUND)
            target_link_libraries(Geo PRIVATE PkgConfig::geographiclib)
            return()
        endif()
    endif()
endif()

include(FetchContent)
FetchContent_Declare(geographiclib
    GIT_REPOSITORY https://github.com/geographiclib/geographiclib.git
    GIT_TAG r2.4
    GIT_SHALLOW TRUE
)
set(BUILD_BOTH_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE INTERNAL "" FORCE)
set(BUILD_MANPAGES OFF CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(geographiclib)

target_link_libraries(Geo PRIVATE GeographicLib::GeographicLib)
