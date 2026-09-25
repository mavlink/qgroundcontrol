# ============================================================================
# Positioning Module
# GPS observations, source health, and GCS position source selection policy
# ============================================================================

qt_add_library(QGCGPSPositioning STATIC GPSObservation.cc GPSPositionBackendAdapter.cc GPSPositionService.cc
               GPSSourceHealth.cc
)
add_library(QGC::GPSPositioning ALIAS QGCGPSPositioning)
set_target_properties(QGCGPSPositioning PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSPositioning PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSPositioning
    PUBLIC QGCGPSCommon QGCGPSSupport QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)
target_sources(
    QGCGPSPositioning
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSObservation.h
           GPSPositionBackendAdapter.h
           GPSPositionService.h
           GPSPositionSourceRegistration.h
           GPSSourceHealth.h
)
