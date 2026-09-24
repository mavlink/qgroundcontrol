# Header-only, so libraries below Core (such as Corrections) can share it without Qt Positioning.
add_library(QGCGPSNotifications INTERFACE)
target_sources(QGCGPSNotifications INTERFACE FILE_SET HEADERS FILES GPSNotificationQueue.h)
target_link_libraries(QGCGPSNotifications INTERFACE Qt6::Core)

qt_add_library(QGCGPSCore STATIC GPSObservation.cc GPSSourceHealth.cc GPSSatelliteStore.cc)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCore
    PUBLIC QGCGPSNotifications QGCGPSReceiver QGCTiming Qt6::Positioning
    PRIVATE QGCLoggingCategory
)

target_sources(QGCGPSCore PUBLIC FILE_SET HEADERS FILES GPSObservation.h GPSSourceHealth.h GPSSatelliteStore.h)
