qt_add_library(
    QGCGPSCore
    STATIC
    GPSConnectionState.cc
    GPSConnectionState.h
    GPSObservation.cc
    GPSObservation.h
    GPSReadTimestamp.h
    GPSSourceHealth.cc
    GPSSourceHealth.h
)

target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCore
    PUBLIC Qt6::Core Qt6::Positioning
    PRIVATE QGCLogging
)
qt_extract_metatypes(QGCGPSCore)
