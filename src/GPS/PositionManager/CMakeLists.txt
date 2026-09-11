add_library(
    QGCGPSPositioning STATIC
    GPSPositionService.cc
    GPSPositionService.h
    GPSPositionSourceRegistration.cc
    GPSPositionSourceRegistration.h
    GPSPositionSourceAdapter.cc
    GPSPositionSourceAdapter.h
    GPSPositionSourceSelector.cc
    GPSPositionSourceSelector.h
)
add_library(QGC::GPSPositioning ALIAS QGCGPSPositioning)
set_target_properties(QGCGPSPositioning PROPERTIES AUTOMOC ON)
target_include_directories(QGCGPSPositioning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSPositioning
    PUBLIC QGCGPSCore Qt6::QmlIntegration
    PRIVATE QGCLoggingCategory
)
