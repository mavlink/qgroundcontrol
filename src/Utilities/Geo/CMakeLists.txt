qt_add_library(QGCGeoMath STATIC QGCGeo.cc QGCGeo.h)

include(GeographicLib.cmake)

target_link_libraries(
    QGCGeoMath
    PUBLIC Qt6::Core Qt6::Gui Qt6::Positioning
    PRIVATE GeographicLib::GeographicLib QGCLogging
)

target_include_directories(QGCGeoMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGeoMath)

add_subdirectory(Formats)
