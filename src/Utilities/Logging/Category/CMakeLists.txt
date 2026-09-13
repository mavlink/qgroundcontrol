if(TARGET QGCLoggingCategory)
    return()
endif()

add_library(QGCLoggingCategory STATIC QGCLoggingCategory.cc)
add_library(QGC::LoggingCategory ALIAS QGCLoggingCategory)
target_link_libraries(QGCLoggingCategory PUBLIC Qt6::Core)

target_sources(QGCLoggingCategory PUBLIC FILE_SET HEADERS FILES QGCLoggingCategory.h)
