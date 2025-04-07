target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DeviceInfo.cc
        DeviceInfo.h
        JsonHelper.cc
        JsonHelper.h
        KML/KMLDomDocument.cc
        KML/KMLDomDocument.h
        KML/KMLHelper.cc
        KML/KMLHelper.h
        QGC.cc
        QGC.h
        FileSystem/QGCCachedFileDownload.cc
        FileSystem/QGCCachedFileDownload.h
        FileSystem/QGCFileDownload.cc
        FileSystem/QGCFileDownload.h
        QGCLoggingCategory.cc
        QGCLoggingCategory.h
        FileSystem/QGCTemporaryFile.cc
        FileSystem/QGCTemporaryFile.h
        Shape/ShapeFileHelper.cc
        Shape/ShapeFileHelper.h
        Shape/SHPFileHelper.cc
        Shape/SHPFileHelper.h
        StateMachine.cc
        StateMachine.h
)

if(ANDROID OR IOS)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE MobileScreenMgr.h)
    if(ANDROID)
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE MobileScreenMgr.cc)
    elseif(IOS)
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE MobileScreenMgr.mm)
    endif()
endif()

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        FileSystem
        KML
        Shape
)

if(QGC_ENABLE_BLUETOOTH)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Bluetooth)
endif()

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

if(Shapelib_ADDED)
    if(TARGET shp)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shp)
    elseif(TARGET shapelib::shp)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shapelib::shp)
    endif()
endif()

#===========================================================================#

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
