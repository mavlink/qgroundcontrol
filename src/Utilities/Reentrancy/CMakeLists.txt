if(TARGET QGCReentrancy)
    return()
endif()

# Guards for QObject owners whose operations call out to observers that may re-enter or delete them.
add_library(QGCReentrancy STATIC NotificationQueue.cc)
add_library(QGC::Reentrancy ALIAS QGCReentrancy)
target_compile_features(QGCReentrancy PUBLIC cxx_std_20)
target_sources(QGCReentrancy PUBLIC FILE_SET HEADERS FILES NotificationQueue.h OperationRevision.h)
target_link_libraries(
    QGCReentrancy
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
