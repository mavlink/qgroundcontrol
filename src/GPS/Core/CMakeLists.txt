qt_add_library(QGCGPSCore STATIC GPSObservation.cc GPSSatelliteObservation.cc GPSSourceHealth.cc GPSSatelliteStore.cc)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCore
    PUBLIC QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)

target_sources(
    QGCGPSCore
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSAltitudeDatum.h
           GPSObservation.h
           GPSSatelliteObservation.h
           GPSSourceHealth.h
           GPSSatelliteStore.h
           GPSSurveyInStatus.h
)
target_sources(
    QGCGPSCore
    PUBLIC FILE_SET
           satellite_types
           TYPE
           HEADERS
           BASE_DIRS
           ..
           FILES
           ../GPSConstellation.h
)
