find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning Qml)

qt_add_library(Utilities STATIC
    JsonHelper.cc
    JsonHelper.h
    QGC.cc
    QGC.h
    QGCCachedFileDownload.cc
	QGCCachedFileDownload.h
    QGCFileDownload.cc
    QGCFileDownload.h
    QGCLoggingCategory.cc
    QGCLoggingCategory.h
    QGCQGeoCoordinate.cc
    QGCQGeoCoordinate.h
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
		Qt6::Qml
		shp
		FactSystem
		Geo
		QGC
		QmlControls
		Settings
	PUBLIC
		Qt6::Core
		Qt6::Network
		Qt6::Positioning
)

target_include_directories(Utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
