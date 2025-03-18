find_package(Qt6 REQUIRED COMPONENTS Bluetooth Core Gui Network Positioning Sensors Qml Xml)

target_sources(QGC
    PRIVATE
        DeviceInfo.cc
        DeviceInfo.h
        JsonHelper.cc
        JsonHelper.h
        KMLDomDocument.cc
        KMLDomDocument.h
        KMLHelper.cc
        KMLHelper.h
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
        ShapeFileHelper.cc
        ShapeFileHelper.h
        SHPFileHelper.cc
        SHPFileHelper.h
        StateMachine.cc
        StateMachine.h
)

if(ANDROID OR IOS)
    target_sources(QGC PRIVATE MobileScreenMgr.h)
    if(ANDROID)
        target_sources(QGC PRIVATE MobileScreenMgr.cc)
    elseif(IOS)
        target_sources(QGC PRIVATE MobileScreenMgr.mm)
    endif()
endif()

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} FileSystem)

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Positioning
        Qt6::Sensors
        Qt6::Xml
)

if(QGC_ENABLE_BLUETOOTH)
    target_link_libraries(QGC PRIVATE Qt6::Bluetooth)
endif()

#===========================================================================#

include(CPM)
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
        target_link_libraries(QGC PRIVATE shp)
    elseif(TARGET shapelib::shp)
        target_link_libraries(QGC PRIVATE shapelib::shp)
    endif()
endif()

#===========================================================================#

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
