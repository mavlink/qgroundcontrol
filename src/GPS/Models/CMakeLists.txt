add_library(QGCGPSModels STATIC GPSSatelliteModel.cc GPSRelativePositionModel.cc)
add_library(QGC::GPSModels ALIAS QGCGPSModels)
set_target_properties(QGCGPSModels PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSModels
    PUBLIC QGCGPSCore
    PRIVATE QGCLoggingCategory
)

target_sources(QGCGPSModels PUBLIC FILE_SET HEADERS FILES GPSRelativePositionModel.h GPSSatelliteModel.h)
