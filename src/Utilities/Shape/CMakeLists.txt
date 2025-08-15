target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        KMLDomDocument.cc
        KMLDomDocument.h
        KMLHelper.cc
        KMLHelper.h
        ShapeFileHelper.cc
        ShapeFileHelper.h
        SHPFileHelper.cc
        SHPFileHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

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

#===========================================================================#

CPMAddPackage(
    NAME PROJ
    VERSION 9.6.2
    GITHUB_REPOSITORY OSGeo/PROJ
    GIT_TAG 9.6.2
    OPTIONS
        "BUILD_EXAMPLES OFF"
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "EMBED_RESOURCE_FILES ON"
)

set(PROJ_INCLUDE_DIR "${PROJ_SOURCE_DIR}/include" CACHE PATH "")
set(PROJ_LIBRARY     "${PROJ_BINARY_DIR}/src/libproj.a" CACHE FILEPATH "")
# Some finders also look at these hints:
set(PROJ_ROOT "${PROJ_BINARY_DIR}" CACHE PATH "")
set(PROJ_DIR  "${PROJ_BINARY_DIR}" CACHE PATH "")
# Optional, helps version checks in some setups:
set(PROJ_VERSION "9.6.2" CACHE STRING "")

#===========================================================================#

CPMAddPackage(
    NAME GDAL
    GITHUB_REPOSITORY OSGeo/gdal
    VERSION 3.11.3
    OPTIONS
        "BUILD_APPS OFF"
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "GDAL_BUILD_OPTIONAL_DRIVERS OFF"
        "OGR_BUILD_OPTIONAL_DRIVERS OFF"
        "GDAL_USE_HDFS OFF"
        "CMAKE_DISABLE_FIND_PACKAGE_HDFS ON"
        "GDAL_USE_ZLIB_INTERNAL ON"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GDAL::GDAL)
