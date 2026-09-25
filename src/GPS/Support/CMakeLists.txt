add_library(QGCGPSSupport STATIC GPSNotificationQueue.cc)
target_compile_features(QGCGPSSupport PUBLIC cxx_std_20)
target_sources(QGCGPSSupport PUBLIC FILE_SET HEADERS FILES GPSNotificationQueue.h GPSRevision.h)
target_link_libraries(
    QGCGPSSupport
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
