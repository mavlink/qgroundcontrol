qt_add_library(
    QGCGPSCore
    STATIC
    GPSObservation.cc
    GPSPositionPolicy.cc
    GPSSatelliteObservation.cc
    GPSSourceHealth.cc
    GPSSatelliteStore.cc
)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCore
    PUBLIC QGC::GPSNativeContracts QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)

target_sources(
    QGCGPSCore
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSObservation.h
           GPSPositionPolicy.h
           GPSSatelliteObservation.h
           GPSSourceHealth.h
           GPSSatelliteStore.h
           GPSSurveyInStatus.h
)
