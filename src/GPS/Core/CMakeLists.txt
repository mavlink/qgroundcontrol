add_library(
    QGCGPSCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSObservation.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSRelativePositionStore.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSIntegrityStore.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSSatelliteStore.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSourceHealth.cc
)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
qt_extract_metatypes(QGCGPSCore)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCore
    PUBLIC QGCGPSNativeContracts QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)

target_sources(
    QGCGPSCore
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSBaseReference.h
           GPSIntegrityObservation.h
           GPSIntegrityStore.h
           GPSObservation.h
           GPSRelativePositionStore.h
           GPSSatelliteStore.h
           GPSSourceHealth.h
           GPSSurveyInStatus.h
)
