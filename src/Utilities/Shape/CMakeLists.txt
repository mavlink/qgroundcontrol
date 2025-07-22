target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GDALHelper.cc
        GDALHelper.h
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

# find_package(PROJ REQUIRED CONFIG)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PROJ::proj)
# list(APPEND CMAKE_PREFIX_PATH "${proj_BINARY_DIR}")

#===========================================================================#

CPMAddPackage(
    NAME GDAL
    GITHUB_REPOSITORY OSGeo/gdal
    VERSION 3.11.1
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
