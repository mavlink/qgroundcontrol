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
    target_sources(Utilities
        PRIVATE
            MobileScreenMgr.cc
            MobileScreenMgr.h
    )
endif()

set(BUILD_SHAPELIB_CONTRIB OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(BUILD_APPS OFF CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(shapelib
    GIT_REPOSITORY https://github.com/OSGeo/shapelib.git
    GIT_TAG 6fe5012d0d893964943befc2034a055d5aa588a8
)
FetchContent_GetProperties(shapelib)
if(NOT shapelib_POPULATED)
    FetchContent_Populate(shapelib)
    add_subdirectory(${shapelib_SOURCE_DIR} ${shapelib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

target_link_libraries(Utilities
    PRIVATE
        Qt6::Bluetooth
        Qt6::Qml
        shp
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

target_include_directories(Utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
