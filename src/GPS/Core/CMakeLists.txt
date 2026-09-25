add_library(QGCGPSNotifications STATIC GPSNotificationQueue.cc)
target_compile_features(QGCGPSNotifications PUBLIC cxx_std_20)
target_sources(QGCGPSNotifications PUBLIC FILE_SET HEADERS FILES GPSNotificationQueue.h GPSRevision.h)
target_link_libraries(
    QGCGPSNotifications
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)

qt_add_library(QGCGPSCore STATIC GPSObservation.cc GPSSourceHealth.cc)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
# Receiver reports are needed only to build observations from them.
target_link_libraries(
    QGCGPSCore
    PUBLIC QGCGPSNotifications QGCTiming Qt6::Positioning
    PRIVATE QGCGPSReceiver QGCLoggingCategory
)

target_sources(QGCGPSCore PUBLIC FILE_SET HEADERS FILES GPSObservation.h GPSSourceHealth.h)
target_sources(
    QGCGPSCore
    PUBLIC FILE_SET
           fix_quality_types
           TYPE
           HEADERS
           BASE_DIRS
           ..
           FILES
           ../GPSFixQuality.h
)
