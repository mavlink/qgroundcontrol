set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceInfo.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceInfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JsonHelper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/JsonHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/KML/KMLDomDocument.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/KML/KMLDomDocument.h
    ${CMAKE_CURRENT_SOURCE_DIR}/KML/KMLHelper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/KML/KMLHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGC.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCCachedFileDownload.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCCachedFileDownload.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCFileDownload.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCFileDownload.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCLoggingCategory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCLoggingCategory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCTemporaryFile.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCTemporaryFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Shape/ShapeFileHelper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Shape/ShapeFileHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Shape/SHPFileHelper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Shape/SHPFileHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StateMachine.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StateMachine.h
    CACHE INTERNAL "" FORCE
)

if(IOS)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/MobileScreenMgr.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/MobileScreenMgr.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Qt6::Sensors
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem
    ${CMAKE_CURRENT_SOURCE_DIR}/KML
    ${CMAKE_CURRENT_SOURCE_DIR}/Shape
    CACHE INTERNAL "" FORCE
)

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

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    shp
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
