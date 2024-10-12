add_subdirectory(Compression)

find_package(Qt6 REQUIRED COMPONENTS Bluetooth Core Gui Network Positioning Sensors Qml Xml)

qt_add_library(Utilities STATIC
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
    QGCCachedFileDownload.cc
    QGCCachedFileDownload.h
    QGCFileDownload.cc
    QGCFileDownload.h
    QGCLoggingCategory.cc
    QGCLoggingCategory.h
    QGCTemporaryFile.cc
    QGCTemporaryFile.h
    ShapeFileHelper.cc
    ShapeFileHelper.h
    SHPFileHelper.cc
    SHPFileHelper.h
    StateMachine.cc
    StateMachine.h
)

if(MOBILE)
    target_sources(Utilities PRIVATE MobileScreenMgr.h)
    if(ANDROID)
        target_sources(Utilities PRIVATE MobileScreenMgr.cc)
    elseif(IOS)
        target_sources(Utilities PRIVATE MobileScreenMgr.mm)
    endif()
endif()

target_include_directories(Utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Utilities
    PRIVATE
        Qt6::Qml
        FactSystem
        Geo
        QGC
        QmlControls
        Settings
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Positioning
        Qt6::Sensors
        Qt6::Xml
)

if(QGC_ENABLE_BLUETOOTH)
    target_link_libraries(Utilities PRIVATE Qt6::Bluetooth)
endif()

set(MINIMUM_SHAPELIB_VERSION 1.6.0)

if(NOT QGC_BUILD_DEPENDENCIES)
    find_package(Shapelib ${MINIMUM_SHAPELIB_VERSION})
    if(Shapelib_FOUND)
        message(STATUS "Found Shapelib ${Shapelib_VERSION}")
        target_link_libraries(Utilities PRIVATE SHAPELIB::shp)
    endif()
endif()

if(NOT Shapelib_FOUND)
    message(STATUS "Building Shapelib")
    include(FetchContent)
    FetchContent_Declare(shapelib
        GIT_REPOSITORY https://github.com/OSGeo/shapelib.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    set(BUILD_SHAPELIB_CONTRIB OFF CACHE INTERNAL "")
    set(BUILD_APPS OFF CACHE INTERNAL "")
    set(saved_BUILD_TESTING ${BUILD_TESTING})
    set(BUILD_TESTING OFF CACHE INTERNAL "Disable testing for Shapelib")
    FetchContent_MakeAvailable(shapelib)
    set(BUILD_TESTING ${saved_BUILD_TESTING} CACHE INTERNAL "Restore BUILD_TESTING")
    target_link_libraries(Utilities PRIVATE shp)
endif()
