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

# CPMAddPackage(
#     NAME PROJ
#     VERSION 9.6.0
#     GITHUB_REPOSITORY OSGeo/PROJ
#     GIT_TAG 9.6.0
#     OPTIONS
#         "BUILD_EXAMPLES OFF"
#         "BUILD_SHARED_LIBS OFF"
#         "BUILD_TESTING OFF"
# )

CPMAddPackage(
    NAME GDAL
    GITHUB_REPOSITORY OSGeo/gdal
    VERSION 3.11.0
    OPTIONS
        "BUILD_APPS OFF"
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "GDAL_BUILD_OPTIONAL_DRIVERS OFF"
        "OGR_BUILD_OPTIONAL_DRIVERS OFF"
)

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GDAL::gdal)
