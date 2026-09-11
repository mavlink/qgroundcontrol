add_library(QGCGPSModels STATIC GPSSatelliteModel.cc GPSSatelliteModel.h GPSRelativePositionModel.cc
                                GPSRelativePositionModel.h
)
add_library(QGC::GPSModels ALIAS QGCGPSModels)
set_target_properties(QGCGPSModels PROPERTIES AUTOMOC ON)
target_include_directories(QGCGPSModels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSModels
    PUBLIC QGCGPSCore
    PRIVATE QGCLoggingCategory
)
