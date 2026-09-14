add_library(QGCGPSCore STATIC GPSObservation.cc GPSSourceHealth.cc GPSSatelliteStore.cc)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
qt_extract_metatypes(QGCGPSCore)
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
           GPSObservation.h
           GPSReceiverTypes.h
           GPSSourceHealth.h
           GPSConstellation.h
           GPSSatelliteStore.h
)
