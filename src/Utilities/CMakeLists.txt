find_package(Qt6 REQUIRED COMPONENTS Core Positioning Qml)

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
)

if(MOBILE)
	target_sources(Utilities
		PRIVATE
			MobileScreenMgr.cc
        	MobileScreenMgr.h
	)
endif()

target_link_libraries(Utilities
	PRIVATE
		Qt6::Qml
		shp
	PUBLIC
		Qt6::Core
		Qt6::Positioning
		qgc
)

target_include_directories(Utilities
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/shapelib
)
