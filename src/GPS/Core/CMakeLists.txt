add_library(
    QGCGPSCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConnectionControl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConnectionControl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConnectionState.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConnectionState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSObservation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSObservation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSBaseReference.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSIntegrityObservation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSurveyInStatus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRelativePositionStore.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRelativePositionStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSIntegrityStore.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSIntegrityStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSatelliteStore.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSatelliteStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSourceHealth.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSSourceHealth.h
)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSCore
    PUBLIC QGCIO QGCGPSContracts QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)
